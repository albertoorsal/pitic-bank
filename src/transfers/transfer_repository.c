/* transfer_repository.c - Dispatch helpers shared by every repository backend. */
#include "transfer_repository.h"

RepoStatus transfer_repo_find_by_account_id(const TransferRepository *repo, long account_id, TransferList *out)
{
    return repo->find_by_account_id(repo->self, account_id, out);
}

RepoStatus transfer_repo_find_all(const TransferRepository *repo, TransferList *out)
{
    return repo->find_all(repo->self, out);
}

void transfer_repo_destroy(TransferRepository *repo)
{
    if (repo != NULL && repo->destroy != NULL) {
        repo->destroy(repo->self);
    }
}
