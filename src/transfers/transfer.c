/* transfer.c - In-memory storage for a growable list of Transfer values. */
#include "transfer.h"
#include <stdlib.h>

enum { TRANSFER_LIST_INITIAL_CAPACITY = 8 };

void transfer_list_init(TransferList *list)
{
    list->items = NULL;
    list->count = 0;
    list->capacity = 0;
}

int transfer_list_append(TransferList *list, const Transfer *transfer)
{
    if (list->count == list->capacity) {
        size_t new_capacity = list->capacity == 0
            ? TRANSFER_LIST_INITIAL_CAPACITY
            : list->capacity * 2;

        Transfer *grown = realloc(list->items, new_capacity * sizeof(Transfer));
        if (grown == NULL) {
            return -1;
        }

        list->items = grown;
        list->capacity = new_capacity;
    }

    list->items[list->count] = *transfer;
    list->count++;
    return 0;
}

void transfer_list_free(TransferList *list)
{
    free(list->items);
    list->items = NULL;
    list->count = 0;
    list->capacity = 0;
}
