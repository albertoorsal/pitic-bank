/*
 * pg_user_repository.h - Concrete PostgreSQL (libpq) implementation of the
 * UserRepository interface.
 *
 * Callers include this file ONLY at the composition root (main). Everything
 * else in the program depends on the abstract user_repository.h, so libpq
 * stays a swappable detail.
 */
#ifndef PG_USER_REPOSITORY_H
#define PG_USER_REPOSITORY_H
 
#include "user_repository.h"

/*
 * Build a repository backed by PostgreSQL.
 *
 * `conn_info` is a standard libpq connection string, e.g.
 *   "host=localhost dbname=appdb user=app password=secret".
 *
 * On success returns REPO_OK and fills `out_repo` with a ready-to-use vtable.
 * The caller owns the repository and must release it with repo_destroy().
 * On failure returns REPO_ERROR and prints the driver's diagnostic.
 */
RepoStatus pg_user_repository_create(const char *conn_info, UserRepository *out_repo);

#endif