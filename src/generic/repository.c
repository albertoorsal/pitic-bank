/* repository.c - Shared plumbing for every PostgreSQL-backed repository. */
#include "repository.h"
#include <stdio.h>
#include <string.h>

RepoStatus classify_failure(PGconn *conn, PGresult *result)
{
    const char *sqlstate = PQresultErrorField(result, PG_DIAG_SQLSTATE);

    if (sqlstate != NULL && strcmp(sqlstate, SQLSTATE_UNIQUE_VIOLATION) == 0)
    {
        return REPO_CONFLICT;
    }

    fprintf(stderr, "[pg] query failed: %s", PQerrorMessage(conn));
    return REPO_ERROR;
}

const char *id_param(long id, char *buffer, size_t size)
{
    snprintf(buffer, size, "%ld", id);
    return buffer;
}

const char *repo_status_text(RepoStatus status)
{
    switch (status) {
        case REPO_OK:        return "OK";
        case REPO_NOT_FOUND: return "not found";
        case REPO_CONFLICT:  return "conflict";
        case REPO_ERROR:     return "storage error";
    }
    return "unknown status";
}
