/*
 * pg_transfer_repository.h - Concrete PostgreSQL (libpq) implementation of
 * the TransferRepository interface.
 *
 * Callers include this file ONLY at the composition root (main). Everything
 * else in the program depends on the abstract transfer_repository.h, so
 * libpq stays a swappable detail.
 */
#ifndef PG_TRANSFER_REPOSITORY_H
#define PG_TRANSFER_REPOSITORY_H

#include "transfer_repository.h"

RepoStatus pg_transfer_repository_create(const char *conn_info, TransferRepository *out_repo);

#endif
