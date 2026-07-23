/*
 * pg_transfer_repository.c - PostgreSQL-backed TransferRepository.
 *
 * Every query joins accounts twice (once for the sender, once for the
 * receiver) to surface human-readable account numbers instead of raw ids.
 */
#include "pg_transfer_repository.h"
#include <libpq-fe.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum {
    COL_ID = 0,
    COL_FROM_ACCOUNT_ID = 1,
    COL_FROM_ACCOUNT_NUMBER = 2,
    COL_TO_ACCOUNT_ID = 3,
    COL_TO_ACCOUNT_NUMBER = 4,
    COL_AMOUNT = 5,
    COL_CREATED_AT = 6
};

static const char *SELECT_SQL =
    "SELECT t.id, t.from_account_id, fa.account_number, "
    "       t.to_account_id, ta.account_number, "
    "       t.amount_cents, t.created_at "
    "FROM transfers t "
    "JOIN accounts fa ON fa.id = t.from_account_id "
    "JOIN accounts ta ON ta.id = t.to_account_id ";

static void row_to_transfer(const PGresult *result, int row, Transfer *out)
{
    out->id = strtol(PQgetvalue(result, row, COL_ID), NULL, 10);
    out->from_account_id = strtol(PQgetvalue(result, row, COL_FROM_ACCOUNT_ID), NULL, 10);
    out->to_account_id = strtol(PQgetvalue(result, row, COL_TO_ACCOUNT_ID), NULL, 10);
    out->amount_cents = strtoll(PQgetvalue(result, row, COL_AMOUNT), NULL, 10);

    snprintf(out->from_account_number, sizeof(out->from_account_number), "%s",
        PQgetvalue(result, row, COL_FROM_ACCOUNT_NUMBER)
    );
    snprintf(out->to_account_number, sizeof(out->to_account_number), "%s",
        PQgetvalue(result, row, COL_TO_ACCOUNT_NUMBER)
    );
    snprintf(out->created_at, sizeof(out->created_at), "%s",
        PQgetvalue(result, row, COL_CREATED_AT)
    );
}

static RepoStatus pg_find_by_account_id(void *self, long account_id, TransferList *out)
{
    PGconn *conn = self;
    char sql[512];
    snprintf(sql, sizeof(sql),
        "%s WHERE t.from_account_id = $1 OR t.to_account_id = $1 ORDER BY t.id DESC",
        SELECT_SQL);

    char id_text[32];
    const char *params[] = { id_param(account_id, id_text, sizeof(id_text)) };

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
        Transfer transfer;
        row_to_transfer(result, row, &transfer);
        if (transfer_list_append(out, &transfer) != 0)
        {
            PQclear(result);
            return REPO_ERROR;
        }
    }

    PQclear(result);
    return REPO_OK;
}

static RepoStatus pg_find_all(void *self, TransferList *out)
{
    PGconn *conn = self;
    char sql[512];
    snprintf(sql, sizeof(sql), "%s ORDER BY t.id DESC", SELECT_SQL);

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
        Transfer transfer;
        row_to_transfer(result, row, &transfer);
        if (transfer_list_append(out, &transfer) != 0)
        {
            PQclear(result);
            return REPO_ERROR;
        }
    }

    PQclear(result);
    return REPO_OK;
}

static void pg_destroy(void *self)
{
    PQfinish((PGconn *)self);
}

RepoStatus pg_transfer_repository_create(const char *conn_info, TransferRepository *out_repo)
{
    PGconn *conn = PQconnectdb(conn_info);

    if (PQstatus(conn) != CONNECTION_OK) {
        fprintf(stderr, "[pg] connection failed: %s", PQerrorMessage(conn));
        PQfinish(conn);
        return REPO_ERROR;
    }

    out_repo->self = conn;
    out_repo->find_by_account_id = pg_find_by_account_id;
    out_repo->find_all = pg_find_all;
    out_repo->destroy = pg_destroy;
    return REPO_OK;
}
