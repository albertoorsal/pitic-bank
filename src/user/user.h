/**
 * @file user.h
 * @author Alberto Ornelas
 * @brief Domain entity: User
 *  This module only kwnos what a User *is* and how to manage
 *  collections of users in memory. It has no knowledge of database.
 *  the CLI, or bussines logic.
 */
#ifndef USER_H
#define USER_H

#include <stddef.h>

enum {
    USER_NAME_MAX = 128,
    USER_EMAIL_MAX = 256,
    USER_PASSWORD_MAX = 128,
    USER_ROLE_MAX = 32,
    USER_RFC_MAX = 13
};

/* A single doamin entity. id == 0 means "not yet persisted" */
typedef struct {
    long id;
    char name[USER_NAME_MAX];
    char email[USER_EMAIL_MAX];
    char password_hash[USER_PASSWORD_MAX];
    char role[USER_ROLE_MAX];
    char rfc[USER_RFC_MAX];
} User;


/* A groable, owing colleciton of users. */
typedef struct {
    User *items;
    size_t count;
    size_t capacity;
} UserList;

// Initialise an empty list, Safe to call before any append.
void user_list_init(UserList *list);

// Append a copy of 'user'. Returns 0 on success, -1 on colletion failure.
int user_list_append(UserList *list, const User *user);

// Release the list's backing storage and reset it to empty
void user_list_free(UserList *list);


#endif