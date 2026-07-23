/*
 * pg_user_repository.c - PostgreSQL-backed UserRepository.
 *
 * Design notes:
 *   - Every query is parameterised (PQexecParams), never string-concatenated,
 *     so user input can never be interpreted as SQL (no injection).
 *   - Driver-specific errors are translated into the small, backend-neutral
 *     RepoStatus vocabulary, so callers stay decoupled from libpq.
 *   - Each function does exactly one thing; shared plumbing is factored into
 *     private helpers below.
 */
#include "pg_user_repository.h"
#include <libpq-fe.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* colmun ordering shared by every SELECT/RETURNING statement below */
enum { COL_ID = 0, COL_NAME = 1, COL_EMAIL = 2, COL_PASSWORD_HASH = 3, COL_ROLE = 4, COL_RFC = 5 };

/* Private helpers*/

/* Copy one result row into a User struct. */
static void row_to_user(const PGresult *result, int row, User *out)
{
    out->id = strtol(PQgetvalue((PGresult *)result, row, COL_ID), NULL, 10);

    snprintf(out->name, sizeof(out->name), "%s",
        PQgetvalue((PGresult *)result, row, COL_NAME)
    );
    snprintf(out->email, sizeof(out->email), "%s",
        PQgetvalue((PGresult *)result, row, COL_EMAIL)
    );
    snprintf(out->password_hash, sizeof(out->password_hash), "%s",
        PQgetvalue((PGresult *)result, row, COL_PASSWORD_HASH)
    );
    snprintf(out->role, sizeof(out->role), "%s",
        PQgetvalue((PGresult *)result, row, COL_ROLE)
    );
    snprintf(out->rfc, sizeof(out->rfc), "%s",
        PQgetisnull((PGresult *)result, row, COL_RFC) ? "" : PQgetvalue((PGresult *)result, row, COL_RFC)
    );
}

/** CRUD OPERATIONS */
static RepoStatus pg_create(void *self, const User *input, User *created)
{
    PGconn *conn = self;
    const char *sql =
        "INSERT INTO users (name, email, password_hash, role, rfc) "
        "VALUES ($1, $2, crypt($3, gen_salt('bf')), $4, NULLIF($5, '')) "
        "RETURNING id, name, email, password_hash, role, rfc";
    const char *role = (input->role[0] != '\0') ? input->role : "customer";
    const char *params[] = { input->name, input->email, input->password_hash, role, input->rfc };

    PGresult *result = PQexecParams(conn, sql, 5, NULL, params, NULL, NULL, 0);

    if (PQresultStatus(result) != PGRES_TUPLES_OK)
    {
        RepoStatus status = classify_failure(conn, result);
        PQclear(result);
        return status;
    }

    row_to_user(result, 0, created);
    PQclear(result);
    return REPO_OK;
}


static RepoStatus pg_find_by_id(void *self, long id, User *out)
{
    PGconn *conn = self;
    char id_text[32];
    const char *sql = "SELECT id, name, email, password_hash, role, rfc FROM users WHERE id = $1";
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
        row_to_user(result, 0, out);
        status = REPO_OK;
    }

    PQclear(result);
    return status;
}

static RepoStatus pg_find_by_email(void *self, const char *email, User *out)
{
    PGconn *conn = self;
    const char *sql = "SELECT id, name, email, password_hash, role, rfc FROM users WHERE email = $1";
    const char *params[] = { email };

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
        row_to_user(result, 0, out);
        status = REPO_OK;
    }

    PQclear(result);
    return status;
}

/* Verify credentials server-side: pgcrypto's crypt(password, password_hash)
 * re-hashes `password` with the salt embedded in the stored hash and
 * compares the result, so the plaintext hash never has to be pulled into
 * the application to be compared in C. */
static RepoStatus pg_verify_credentials(void *self, const char *email, const char *password, User *out)
{
    PGconn *conn = self;
    const char *sql =
        "SELECT id, name, email, password_hash, role, rfc FROM users "
        "WHERE email = $1 AND password_hash = crypt($2, password_hash)";
    const char *params[] = { email, password };

    PGresult *result = PQexecParams(conn, sql, 2, NULL, params, NULL, NULL, 0);

    if (PQresultStatus(result) != PGRES_TUPLES_OK)
    {
        RepoStatus status = classify_failure(conn, result);
        PQclear(result);
        return status;
    }

    RepoStatus status = REPO_NOT_FOUND;
    if (PQntuples(result) == 1)
    {
        row_to_user(result, 0, out);
        status = REPO_OK;
    }

    PQclear(result);
    return status;
}

static RepoStatus pg_find_all(void *self, UserList *out)
{
    PGconn *conn = self;
    const char *sql = "SELECT id, name, email, password_hash, role, rfc FROM users ORDER BY id";
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
        User user;
        row_to_user(result, row, &user);
        if (user_list_append(out, &user) != 0)
        {
            PQclear(result);
            return REPO_ERROR;
        }
    }
 
    PQclear(result);
    return REPO_OK;
}


static RepoStatus pg_update(void *self, const User *input)
{
    PGconn *conn = self;
    char id_text[32];
    const char *sql =
        "UPDATE users SET name = $1, email = $2 WHERE id = $3";

    const char *params[] = {
        input->name,
        input->email,
        id_param(input->id, id_text, sizeof(id_text))
    };

    PGresult *result = PQexecParams(conn, sql, 3, NULL, params, NULL, NULL, 0);

    if (PQresultStatus(result) != PGRES_COMMAND_OK)
    {
        RepoStatus status = classify_failure(conn, result);
        PQclear(result);
        return status;
    }

    RepoStatus status = 
        (atoi(PQcmdTuples(result)) == 0) ? REPO_NOT_FOUND : REPO_OK;
    PQclear(result);
    return status;
}

static RepoStatus pg_remove(void *self, long id)
{
    PGconn *conn = self;
    char id_text[32];
    const char *sql = "DELETE FROM users WHERE id = $1";
    const char *params[] = { id_param(id, id_text, sizeof(id_text)) };

    PGresult *result = PQexecParams(conn, sql, 1, NULL, params, NULL, NULL, 0);

    if (PQresultStatus(result ) != PGRES_COMMAND_OK)
    {
        RepoStatus status = classify_failure(conn, result);
        PQclear(result);
        return status;
    }

    RepoStatus status =
        (atoi(PQcmdTuples(result)) == 0) ? REPO_NOT_FOUND : REPO_OK;
    PQclear(result);
    return status;
}


static void pg_destroy(void *self)
{
    PQfinish((PGconn *)self);
}


// Construction
RepoStatus pg_user_repository_create(const char *conn_info, UserRepository *out_repo)
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
    out_repo->find_by_email = pg_find_by_email;
    out_repo->verify_credentials = pg_verify_credentials;
    out_repo->find_all = pg_find_all;
    out_repo->update = pg_update;
    out_repo->remove = pg_remove;
    out_repo->destroy = pg_destroy;
    return REPO_OK;
}

