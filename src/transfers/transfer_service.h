/*
 * transfer_service.h - Application layer for transfer history.
 *
 * Money movement itself is orchestrated by AccountService::transfer (it
 * needs both accounts locked together); this service only exposes read
 * access to the resulting history for the CLI.
 */
#ifndef TRANSFER_SERVICE_H
#define TRANSFER_SERVICE_H

#include "transfer.h"
#include "transfer_repository.h"

typedef enum {
    TRANSFER_SERVICE_OK,
    TRANSFER_SERVICE_ERROR
} TransferServiceStatus;

typedef struct {
    const TransferRepository *repository;
} TransferService;

void transfer_service_init(TransferService *service, const TransferRepository *repository);

/* Fetch the transfer history involving `account_id` (as sender or receiver). */
TransferServiceStatus transfer_service_list_for_account(
    const TransferService *service, long account_id, TransferList *out
);

/* Fetch every transfer ever recorded (admin view). */
TransferServiceStatus transfer_service_list_all(const TransferService *service, TransferList *out);

const char *transfer_service_status_text(TransferServiceStatus status);

#endif /* TRANSFER_SERVICE_H */
