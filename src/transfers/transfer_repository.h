/**
 * @file transfer_repository.h
 * @author Alberto Ornelas
 * @brief Transfer repository operations (read-only: transfers are created
 *  as part of AccountRepository::transfer, not through this interface).
 */
#ifndef TRANSFER_REPOSITORY_H
#define TRANSFER_REPOSITORY_H

#include "transfer.h"
#include "generic/repository.h"

typedef struct TransferRepository TransferRepository;

struct TransferRepository {
    /* Implementation-private state (e.g. the live PGconn) */
    void *self;

    /* Append every transfer involving `account_id` (as sender or receiver)
     * to `out` (already initialised by the caller), newest first. */
    RepoStatus (*find_by_account_id)(void *self, long account_id, TransferList *out);

    /* Append every stored transfer to `out` (already initialised by the
     * caller), newest first. Admin-only view. */
    RepoStatus (*find_all)(void *self, TransferList *out);

    /* Release all resources owned by this repository (e.g. close PGconn). */
    void (*destroy)(void *self);
};

RepoStatus transfer_repo_find_by_account_id(const TransferRepository *repo, long account_id, TransferList *out);
RepoStatus transfer_repo_find_all(const TransferRepository *repo, TransferList *out);
void       transfer_repo_destroy(TransferRepository *repo);

#endif /* TRANSFER_REPOSITORY_H */
