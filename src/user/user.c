/* user.c - In-memory storage for a growable list of User values. */
#include "user.h"
#include <stdlib.h>
#include <string.h>

enum { USER_LIST_INITIAL_CAPACITY = 8 };

void user_list_init(UserList *list)
{
    list->items = NULL;
    list->count = 0;
    list->capacity = 0;
}

int user_list_append(UserList *list, const User *user)
{
    if (list->count == list->capacity) {
        size_t new_capacity = list->capacity == 0
            ? USER_LIST_INITIAL_CAPACITY
            : list->capacity * 2;

        User *grown = realloc(list->items, new_capacity * sizeof(User));
        if (grown == NULL) {
            return -1;
        }

        list->items = grown;
        list->capacity = new_capacity;
    }

    list->items[list->count] = *user;
    list->count++;
    return 0;
}

void user_list_free(UserList *list)
{
    free(list->items);
    list->items = NULL;
    list->count = 0;
    list->capacity = 0;
}
