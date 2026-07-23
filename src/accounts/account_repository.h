/**
 * @file account_repository.h
 * @author Alberto Ornelas
 * @brief Account repository operations
 * @date 19-July-2026
 */
#ifndef ACCOUNT_REPOSITORY_H
#define ACCOUNT_REPOSITORY_H

#include "account.h"
#include "generic/repository.h"

typedef struct AccountRepository AccountRepository;

struct AccountRepository {
    /* Implementation-private state (e.g. the live PGconn) */
    void *self;

    /** Persist a new account. `input` supplies user_id/account_number/
     * account_type; on REPO_OK `created` receives the stored row. */
    RepoStatus (*create)(void *self, const Account *input, Account *created);

    /* Load one account by id into `out`. REPO_NOT_FOUND if absent. */
    RepoStatus (*find_by_id)(void *self, long id, Account *out);

    /* Load one account by account number into `out`. REPO_NOT_FOUND if absent. */
    RepoStatus (*find_by_account_number)(void *self, const char *account_number, Account *out);

    /* Append every account owned by `user_id` to `out` (already initialised). */
    RepoStatus (*find_by_user_id)(void *self, long user_id, AccountList *out);

    /* Append every stored account to `out` (already initialised by the caller). */
    RepoStatus (*find_all)(void *self, AccountList *out);

    /* Atomically add `delta_cents` (negative for a withdrawal) to the balance
     * of account `id`, rejecting the change if it would go negative.
     * On REPO_OK, `out` holds the account's post-adjustment row. */
    RepoStatus (*adjust_balance)(void *self, long id, long long delta_cents, Account *out);

    /* Move `amount_cents` from `from_id` to `to_id` as a single atomic
     * operation (same-transaction debit + credit + transfer record).
     * REPO_CONFLICT if the source account has insufficient funds.
     * On REPO_OK, `transfer_id` receives the id of the recorded transfer. */
    RepoStatus (*transfer)(void *self, long from_id, long to_id, long long amount_cents, long *transfer_id);

    /* Release all resources owned by this repository (e.g. close PGconn). */
    void (*destroy)(void *self);
};

/**
 * Convenience wrappers so callers write repo_create(repo, ...) instead of
 * repo->create(repo->self, ...). They also centralise the self-dispatch.
 */
RepoStatus account_repo_create(const AccountRepository *repo, const Account *input, Account *created);
RepoStatus account_repo_find_by_id(const AccountRepository *repo, long id, Account *out);
RepoStatus account_repo_find_by_account_number(const AccountRepository *repo, const char *account_number, Account *out);
RepoStatus account_repo_find_by_user_id(const AccountRepository *repo, long user_id, AccountList *out);
RepoStatus account_repo_find_all(const AccountRepository *repo, AccountList *out);
RepoStatus account_repo_adjust_balance(const AccountRepository *repo, long id, long long delta_cents, Account *out);
RepoStatus account_repo_transfer(const AccountRepository *repo, long from_id, long to_id, long long amount_cents, long *transfer_id);
void       account_repo_destroy(AccountRepository *repo);

#endif /* ACCOUNT_REPOSITORY_H */
