/**
 * @file repository.c
 * @author Alberto Ornelas
 * @brief User repository operations
 * @date 19-July-2026
 *
 */
#ifndef CUSTOMER_REPOSITORY_H
#define CUSTOMER_REPOSITORY_H

#include "user.h"

/**
 * Outcome of a repository operation. Callers branch on this, never on errno
 *   or drive-specific codes, keeping them decoupled from the backend,
 */
typedef enum {
    REPO_OK,
    REPO_NOT_FOUND,
    REPO_CONFLICT,
    REPO_ERROR
} RepoStatus;

typedef struct UserRepository UserRepository;

struct UserRepository {
    /* Implementation-private state (e.g the live PGconn) */
    void *self;

    /** Persist a new user 'input' supplies name/email; on REPO_OK 'created'
     *  receives the stored row including
    */
    RepoStatus (*create)(void *self, const User *input, User *created);

    /* Load one user by id into `out`. REPO_NOT_FOUND if absent. */
    RepoStatus (*find_by_id)(void *self, long id, User *out);

    /* Load one user by email into `out`. REPO_NOT_FOUND if absent. Used for login. */
    RepoStatus (*find_by_email)(void *self, const char *email, User *out);

    /* Verify email + plaintext password against the stored hash (checked
     * server-side via pgcrypto, the hash never leaves the database). On
     * REPO_OK, `out` holds the authenticated user. REPO_NOT_FOUND if the
     * email is unknown or the password does not match. */
    RepoStatus (*verify_credentials)(void *self, const char *email, const char *password, User *out);

    /* append every stored user to 'out' (already initialised by the caller) */
    RepoStatus (*find_all)(void *self, UserList *out);

    /** Overwrite the row identified by 'input->id' REPO_NOT_FOUND if absent. */
    RepoStatus (*update)(void *self, const User *input);

    /* Delete the row with the given id. REPO_NOT_FOUND if absent. */
    RepoStatus (*remove)(void *self, long id);

    /* Release all resources owned by this repository (e.g. close PGconn). */
    void(*destroy)(void *self);
};

/**
 * Convenience wrappers so callers write repo_create (repo) instead of
 * repo->create(repo, self) They also centralise the self-dispatch
 */
RepoStatus repo_create(const UserRepository *repo, const User *input, User *created);
RepoStatus repo_find_by_id(const UserRepository *repo, long id, User *out);
RepoStatus repo_find_by_email(const UserRepository *repo, const char *email, User *out);
RepoStatus repo_verify_credentials(const UserRepository *repo, const char *email, const char *password, User *out);
RepoStatus repo_find_all(const UserRepository *repo, UserList *out);
RepoStatus repo_update(const UserRepository *repo, const User *input);
RepoStatus repo_remove(const UserRepository *repo, long id);
void       repo_destroy(UserRepository *repo);


/** Human-readable label for a status, for loggin/UI */
const char *repo_status_text(RepoStatus status);


#endif /* USER_REPOSITORY_H */
