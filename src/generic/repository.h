/**
 * @file repository.h
 * @author Alberto Ornelas
 * @brief Generic repository definitions shared by every backend-specific
 *  repository (User, Account, Transfer, ...).
 */
#ifndef REPOSITORY_H
#define REPOSITORY_H

#include <libpq-fe.h>
#include <stddef.h>

#define SQLSTATE_UNIQUE_VIOLATION "23505"

/**
 * Outcome of a repository operation. Callers branch on this, never on errno
 *   or drive-specific codes, keeping them decoupled from the backend,
 */
typedef enum {
    REPO_OK,
    REPO_NOT_FOUND,
    REPO_CONFLICT,
    REPO_ERROR
} RepoStatus;

/** Human-readable label for a status, for logging/UI */
const char *repo_status_text(RepoStatus status);

/* Map a failed PGresult to a RepoStatus and log the driver's message. */
RepoStatus classify_failure(PGconn *conn, PGresult *result);

/* Render a long id as a text parameter. `buffer` must hold >= 32 chars. */
const char *id_param(long id, char *buffer, size_t size);

#endif /* REPOSITORY_H */
