/*
 * pg_account_repository.c - PostgreSQL-backed AccountRepository.
 *
 * Design notes:
 *   - Every query is parameterised (PQexecParams), never string-concatenated.
 *   - adjust_balance/transfer run inside an explicit BEGIN/COMMIT with
 *     "SELECT ... FOR UPDATE" row locks so concurrent operations on the same
 *     account cannot race past the non-negative-balance check.
 */
#include "pg_account_repository.h"
#include <libpq-fe.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum { COL_ID = 0, COL_USER_ID = 1, COL_ACCOUNT_NUMBER = 2, COL_ACCOUNT_TYPE = 3, COL_STATUS = 4, COL_BALANCE = 5 };

static const char *SELECT_COLUMNS =
    "id, user_id, account_number, account_type, status, balance_cents";

static void row_to_account(const PGresult *result, int row, Account *out)
{
    out->id = strtol(PQgetvalue(result, row, COL_ID), NULL, 10);
    out->user_id = strtol(PQgetvalue(result, row, COL_USER_ID), NULL, 10);

    snprintf(out->account_number, sizeof(out->account_number), "%s",
        PQgetvalue(result, row, COL_ACCOUNT_NUMBER)
    );
    snprintf(out->account_type, sizeof(out->account_type), "%s",
        PQgetvalue(result, row, COL_ACCOUNT_TYPE)
    );
    snprintf(out->status, sizeof(out->status), "%s",
        PQgetvalue(result, row, COL_STATUS)
    );
    out->balance_cents = strtoll(PQgetvalue(result, row, COL_BALANCE), NULL, 10);
}

static void amount_param(long long cents, char *buffer, size_t size)
{
    snprintf(buffer, size, "%lld", cents);
}

static RepoStatus pg_create(void *self, const Account *input, Account *created)
{
    PGconn *conn = self;
    char sql[256];
    snprintf(sql, sizeof(sql),
        "INSERT INTO accounts (user_id, account_number, account_type) "
        "VALUES ($1, $2, $3) "
        "RETURNING %s", SELECT_COLUMNS);

    char user_id_text[32];
    const char *account_type = (input->account_type[0] != '\0') ? input->account_type : "checking";
    const char *params[] = {
        id_param(input->user_id, user_id_text, sizeof(user_id_text)),
        input->account_number,
        account_type
    };

    PGresult *result = PQexecParams(conn, sql, 3, NULL, params, NULL, NULL, 0);

    if (PQresultStatus(result) != PGRES_TUPLES_OK)
    {
        RepoStatus status = classify_failure(conn, result);
        PQclear(result);
        return status;
    }

    row_to_account(result, 0, created);
    PQclear(result);
    return REPO_OK;
}

static RepoStatus pg_find_by_id(void *self, long id, Account *out)
{
    PGconn *conn = self;
    char sql[256];
    snprintf(sql, sizeof(sql), "SELECT %s FROM accounts WHERE id = $1", SELECT_COLUMNS);

    char id_text[32];
    const char *params[] = { id_param(id, id_text, sizeof(id_text)) };

    PGresult *result = PQexecParams(conn, sql, 1, NULL, params, NULL, NULL, 0);

    if (PQresultStatus(result) != PGRES_TUPLES_OK)
    {
        RepoStatus status = classify_failure(conn, result);
        PQclear(result);
        return status;
    }

    RepoStatus status = REPO_NOT_FOUND;
    if (PQntuples(result) == 1)
    {
        row_to_account(result, 0, out);
        status = REPO_OK;
    }

    PQclear(result);
    return status;
}

static RepoStatus pg_find_by_account_number(void *self, const char *account_number, Account *out)
{
    PGconn *conn = self;
    char sql[256];
    snprintf(sql, sizeof(sql), "SELECT %s FROM accounts WHERE account_number = $1", SELECT_COLUMNS);

    const char *params[] = { account_number };

    PGresult *result = PQexecParams(conn, sql, 1, NULL, params, NULL, NULL, 0);

    if (PQresultStatus(result) != PGRES_TUPLES_OK)
    {
        RepoStatus status = classify_failure(conn, result);
        PQclear(result);
        return status;
    }

    RepoStatus status = REPO_NOT_FOUND;
    if (PQntuples(result) == 1)
    {
        row_to_account(result, 0, out);
        status = REPO_OK;
    }

    PQclear(result);
    return status;
}

static RepoStatus pg_find_by_user_id(void *self, long user_id, AccountList *out)
{
    PGconn *conn = self;
    char sql[256];
    snprintf(sql, sizeof(sql), "SELECT %s FROM accounts WHERE user_id = $1 ORDER BY id", SELECT_COLUMNS);

    char id_text[32];
    const char *params[] = { id_param(user_id, id_text, sizeof(id_text)) };

    PGresult *result = PQexecParams(conn, sql, 1, NULL, params, NULL, NULL, 0);

    if (PQresultStatus(result) != PGRES_TUPLES_OK)
    {
        RepoStatus status = classify_failure(conn, result);
        PQclear(result);
        return status;
    }

    int rows = PQntuples(result);
    for (int row = 0; row < rows; row++)
    {
        Account account;
        row_to_account(result, row, &account);
        if (account_list_append(out, &account) != 0)
        {
            PQclear(result);
            return REPO_ERROR;
        }
    }

    PQclear(result);
    return REPO_OK;
}

static RepoStatus pg_find_all(void *self, AccountList *out)
{
    PGconn *conn = self;
    char sql[256];
    snprintf(sql, sizeof(sql), "SELECT %s FROM accounts ORDER BY id", SELECT_COLUMNS);

    PGresult *result = PQexecParams(conn, sql, 0, NULL, NULL, NULL, NULL, 0);

    if (PQresultStatus(result) != PGRES_TUPLES_OK)
    {
        RepoStatus status = classify_failure(conn, result);
        PQclear(result);
        return status;
    }

    int rows = PQntuples(result);
    for (int row = 0; row < rows; row++)
    {
        Account account;
        row_to_account(result, row, &account);
        if (account_list_append(out, &account) != 0)
        {
            PQclear(result);
            return REPO_ERROR;
        }
    }

    PQclear(result);
    return REPO_OK;
}

/* Roll back and translate a mid-transaction failure into a RepoStatus. */
static RepoStatus abort_txn(PGconn *conn, PGresult *result)
{
    RepoStatus status = classify_failure(conn, result);
    PQclear(result);
    PGresult *rollback = PQexec(conn, "ROLLBACK");
    PQclear(rollback);
    return status;
}

/* Lock the row with FOR UPDATE so a concurrent adjust_balance/transfer on the
 * same account must wait, keeping the non-negative-balance check race-free. */
static RepoStatus lock_account_for_update(PGconn *conn, long id, Account *out)
{
    char sql[256];
    snprintf(sql, sizeof(sql), "SELECT %s FROM accounts WHERE id = $1 FOR UPDATE", SELECT_COLUMNS);

    char id_text[32];
    const char *params[] = { id_param(id, id_text, sizeof(id_text)) };

    PGresult *result = PQexecParams(conn, sql, 1, NULL, params, NULL, NULL, 0);

    if (PQresultStatus(result) != PGRES_TUPLES_OK)
    {
        return abort_txn(conn, result);
    }
    if (PQntuples(result) != 1)
    {
        PQclear(result);
        PGresult *rollback = PQexec(conn, "ROLLBACK");
        PQclear(rollback);
        return REPO_NOT_FOUND;
    }

    row_to_account(result, 0, out);
    PQclear(result);
    return REPO_OK;
}

static RepoStatus set_balance(PGconn *conn, long id, long long new_balance, Account *out)
{
    char sql[256];
    snprintf(sql, sizeof(sql), "UPDATE accounts SET balance_cents = $1 WHERE id = $2 RETURNING %s", SELECT_COLUMNS);

    char balance_text[32];
    char id_text[32];
    amount_param(new_balance, balance_text, sizeof(balance_text));
    const char *params[] = { balance_text, id_param(id, id_text, sizeof(id_text)) };

    PGresult *result = PQexecParams(conn, sql, 2, NULL, params, NULL, NULL, 0);

    if (PQresultStatus(result) != PGRES_TUPLES_OK)
    {
        return abort_txn(conn, result);
    }

    row_to_account(result, 0, out);
    PQclear(result);
    return REPO_OK;
}

static RepoStatus pg_adjust_balance(void *self, long id, long long delta_cents, Account *out)
{
    PGconn *conn = self;

    PGresult *begin = PQexec(conn, "BEGIN");
    if (PQresultStatus(begin) != PGRES_COMMAND_OK)
    {
        RepoStatus status = classify_failure(conn, begin);
        PQclear(begin);
        return status;
    }
    PQclear(begin);

    Account current;
    RepoStatus status = lock_account_for_update(conn, id, &current);
    if (status != REPO_OK)
    {
        return status;
    }

    long long new_balance = current.balance_cents + delta_cents;
    if (new_balance < 0)
    {
        PGresult *rollback = PQexec(conn, "ROLLBACK");
        PQclear(rollback);
        return REPO_CONFLICT;
    }

    status = set_balance(conn, id, new_balance, out);
    if (status != REPO_OK)
    {
        return status;
    }

    PGresult *commit = PQexec(conn, "COMMIT");
    if (PQresultStatus(commit) != PGRES_COMMAND_OK)
    {
        RepoStatus commit_status = classify_failure(conn, commit);
        PQclear(commit);
        return commit_status;
    }
    PQclear(commit);
    return REPO_OK;
}

static RepoStatus pg_transfer(void *self, long from_id, long to_id, long long amount_cents, long *transfer_id)
{
    PGconn *conn = self;

    PGresult *begin = PQexec(conn, "BEGIN");
    if (PQresultStatus(begin) != PGRES_COMMAND_OK)
    {
        RepoStatus status = classify_failure(conn, begin);
        PQclear(begin);
        return status;
    }
    PQclear(begin);

    /* Lock accounts in a fixed order (ascending id) to prevent deadlocks
     * between two concurrent transfers that move money in opposite
     * directions between the same pair of accounts. */
    long first_id = from_id < to_id ? from_id : to_id;
    long second_id = from_id < to_id ? to_id : from_id;

    Account first, second;
    RepoStatus status = lock_account_for_update(conn, first_id, &first);
    if (status != REPO_OK)
    {
        return status;
    }
    status = lock_account_for_update(conn, second_id, &second);
    if (status != REPO_OK)
    {
        return status;
    }

    Account *from = (from_id == first_id) ? &first : &second;

    if (from->balance_cents < amount_cents)
    {
        PGresult *rollback = PQexec(conn, "ROLLBACK");
        PQclear(rollback);
        return REPO_CONFLICT;
    }

    Account updated;
    status = set_balance(conn, from_id, from->balance_cents - amount_cents, &updated);
    if (status != REPO_OK)
    {
        return status;
    }

    Account *to = (to_id == first_id) ? &first : &second;
    status = set_balance(conn, to_id, to->balance_cents + amount_cents, &updated);
    if (status != REPO_OK)
    {
        return status;
    }

    char sql[256];
    snprintf(sql, sizeof(sql),
        "INSERT INTO transfers (from_account_id, to_account_id, amount_cents) "
        "VALUES ($1, $2, $3) RETURNING id");

    char from_text[32], to_text[32], amount_text[32];
    amount_param(amount_cents, amount_text, sizeof(amount_text));
    const char *params[] = {
        id_param(from_id, from_text, sizeof(from_text)),
        id_param(to_id, to_text, sizeof(to_text)),
        amount_text
    };

    PGresult *result = PQexecParams(conn, sql, 3, NULL, params, NULL, NULL, 0);
    if (PQresultStatus(result) != PGRES_TUPLES_OK)
    {
        return abort_txn(conn, result);
    }

    *transfer_id = strtol(PQgetvalue(result, 0, 0), NULL, 10);
    PQclear(result);

    PGresult *commit = PQexec(conn, "COMMIT");
    if (PQresultStatus(commit) != PGRES_COMMAND_OK)
    {
        RepoStatus commit_status = classify_failure(conn, commit);
        PQclear(commit);
        return commit_status;
    }
    PQclear(commit);
    return REPO_OK;
}

static void pg_destroy(void *self)
{
    PQfinish((PGconn *)self);
}

// Construction
RepoStatus pg_account_repository_create(const char *conn_info, AccountRepository *out_repo)
{
    PGconn *conn = PQconnectdb(conn_info);

    if (PQstatus(conn) != CONNECTION_OK) {
        fprintf(stderr, "[pg] connection failed: %s", PQerrorMessage(conn));
        PQfinish(conn);
        return REPO_ERROR;
    }

    out_repo->self = conn;
    out_repo->create = pg_create;
    out_repo->find_by_id = pg_find_by_id;
    out_repo->find_by_account_number = pg_find_by_account_number;
    out_repo->find_by_user_id = pg_find_by_user_id;
    out_repo->find_all = pg_find_all;
    out_repo->adjust_balance = pg_adjust_balance;
    out_repo->transfer = pg_transfer;
    out_repo->destroy = pg_destroy;
    return REPO_OK;
}
