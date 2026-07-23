/* transfer_service.c - Read access to transfer history. */
#include "transfer_service.h"

static TransferServiceStatus from_repo_status(RepoStatus status)
{
    return status == REPO_OK ? TRANSFER_SERVICE_OK : TRANSFER_SERVICE_ERROR;
}

void transfer_service_init(TransferService *service, const TransferRepository *repository)
{
    service->repository = repository;
}

TransferServiceStatus transfer_service_list_for_account(
    const TransferService *service, long account_id, TransferList *out)
{
    return from_repo_status(transfer_repo_find_by_account_id(service->repository, account_id, out));
}

TransferServiceStatus transfer_service_list_all(const TransferService *service, TransferList *out)
{
    return from_repo_status(transfer_repo_find_all(service->repository, out));
}

const char *transfer_service_status_text(TransferServiceStatus status)
{
    switch (status) {
        case TRANSFER_SERVICE_OK:    return "OK";
        case TRANSFER_SERVICE_ERROR: return "storage error";
    }
    return "unknown status";
}
