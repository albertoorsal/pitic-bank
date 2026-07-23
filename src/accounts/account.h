/**
 * @file account.h
 * @author Alberto Ornelas
 * @brief Domain entity: Account
 *  This module only knows what an Account *is* and how to manage
 *  collections of accounts in memory. It has no knowledge of the database,
 *  the CLI, or business logic.
 */
#ifndef ACCOUNT_H
#define ACCOUNT_H

#include <stddef.h>

enum {
    ACCOUNT_NUMBER_MAX = 16,
    ACCOUNT_TYPE_MAX = 16,
    ACCOUNT_STATUS_MAX = 16
};

/* Balances and amounts are stored as integer cents to avoid floating-point
 * rounding: 10050 means $100.50. */
typedef struct {
    long id;
    long user_id;
    char account_number[ACCOUNT_NUMBER_MAX];
    char account_type[ACCOUNT_TYPE_MAX];
    char status[ACCOUNT_STATUS_MAX];
    long long balance_cents;
} Account;

/* A growable, owning collection of accounts. */
typedef struct {
    Account *items;
    size_t count;
    size_t capacity;
} AccountList;

/* Initialise an empty list. Safe to call before any append. */
void account_list_init(AccountList *list);

/* Append a copy of `account`. Returns 0 on success, -1 on allocation failure. */
int account_list_append(AccountList *list, const Account *account);

/* Release the list's backing storage and reset it to empty. */
void account_list_free(AccountList *list);

#endif
