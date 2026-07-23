/*
 * pg_account_repository.h - Concrete PostgreSQL (libpq) implementation of the
 * AccountRepository interface.
 *
 * Callers include this file ONLY at the composition root (main). Everything
 * else in the program depends on the abstract account_repository.h, so libpq
 * stays a swappable detail.
 */
#ifndef PG_ACCOUNT_REPOSITORY_H
#define PG_ACCOUNT_REPOSITORY_H

#include "account_repository.h"

/*
 * Build a repository backed by PostgreSQL.
 *
 * `conn_info` is a standard libpq connection string, e.g.
 *   "host=localhost dbname=appdb user=app password=secret".
 *
 * On success returns REPO_OK and fills `out_repo` with a ready-to-use vtable.
 * The caller owns the repository and must release it with account_repo_destroy().
 * On failure returns REPO_ERROR and prints the driver's diagnostic.
 */
RepoStatus pg_account_repository_create(const char *conn_info, AccountRepository *out_repo);

#endif
