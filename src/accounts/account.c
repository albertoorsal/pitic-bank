/* account.c - In-memory storage for a growable list of Account values. */
#include "account.h"
#include <stdlib.h>

enum { ACCOUNT_LIST_INITIAL_CAPACITY = 8 };

void account_list_init(AccountList *list)
{
    list->items = NULL;
    list->count = 0;
    list->capacity = 0;
}

int account_list_append(AccountList *list, const Account *account)
{
    if (list->count == list->capacity) {
        size_t new_capacity = list->capacity == 0
            ? ACCOUNT_LIST_INITIAL_CAPACITY
            : list->capacity * 2;

        Account *grown = realloc(list->items, new_capacity * sizeof(Account));
        if (grown == NULL) {
            return -1;
        }

        list->items = grown;
        list->capacity = new_capacity;
    }

    list->items[list->count] = *account;
    list->count++;
    return 0;
}

void account_list_free(AccountList *list)
{
    free(list->items);
    list->items = NULL;
    list->count = 0;
    list->capacity = 0;
}
