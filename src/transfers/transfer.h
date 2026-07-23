/**
 * @file transfer.h
 * @author Alberto Ornelas
 * @brief Domain entity: Transfer
 *  A single completed movement of money between two accounts. This module
 *  only knows what a Transfer *is* and how to manage collections of them in
 *  memory. It has no knowledge of the database, the CLI, or business logic.
 */
#ifndef TRANSFER_H
#define TRANSFER_H

#include <stddef.h>
#include "accounts/account.h"

typedef struct {
    long id;
    long from_account_id;
    char from_account_number[ACCOUNT_NUMBER_MAX];
    long to_account_id;
    char to_account_number[ACCOUNT_NUMBER_MAX];
    long long amount_cents;
    char created_at[32];
} Transfer;

/* A growable, owning collection of transfers. */
typedef struct {
    Transfer *items;
    size_t count;
    size_t capacity;
} TransferList;

/* Initialise an empty list. Safe to call before any append. */
void transfer_list_init(TransferList *list);

/* Append a copy of `transfer`. Returns 0 on success, -1 on allocation failure. */
int transfer_list_append(TransferList *list, const Transfer *transfer);

/* Release the list's backing storage and reset it to empty. */
void transfer_list_free(TransferList *list);

#endif
