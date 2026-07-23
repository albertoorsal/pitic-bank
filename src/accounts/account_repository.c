/* account_repository.c - Dispatch helpers shared by every repository backend. */
#include "account_repository.h"

RepoStatus account_repo_create(const AccountRepository *repo, const Account *input, Account *created)
{
    return repo->create(repo->self, input, created);
}

RepoStatus account_repo_find_by_id(const AccountRepository *repo, long id, Account *out)
{
    return repo->find_by_id(repo->self, id, out);
}

RepoStatus account_repo_find_by_account_number(const AccountRepository *repo, const char *account_number, Account *out)
{
    return repo->find_by_account_number(repo->self, account_number, out);
}

RepoStatus account_repo_find_by_user_id(const AccountRepository *repo, long user_id, AccountList *out)
{
    return repo->find_by_user_id(repo->self, user_id, out);
}

RepoStatus account_repo_find_all(const AccountRepository *repo, AccountList *out)
{
    return repo->find_all(repo->self, out);
}

RepoStatus account_repo_adjust_balance(const AccountRepository *repo, long id, long long delta_cents, Account *out)
{
    return repo->adjust_balance(repo->self, id, delta_cents, out);
}

RepoStatus account_repo_transfer(const AccountRepository *repo, long from_id, long to_id, long long amount_cents, long *transfer_id)
{
    return repo->transfer(repo->self, from_id, to_id, amount_cents, transfer_id);
}

void account_repo_destroy(AccountRepository *repo)
{
    if (repo != NULL && repo->destroy != NULL) {
        repo->destroy(repo->self);
    }
}
