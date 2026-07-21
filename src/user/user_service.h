
/*
 * user_service.h - Application/business layer.
 *
 * The service enforces the domain rules (a user must have a non-empty name and
 * a plausible email) and orchestrates persistence. Crucially it depends on the
 * UserRepository *abstraction*, not on libpq - so the exact same service runs
 * against Postgres in production and against a mock repository in tests.
 *
 * Separation of concerns:
 *   presentation (CLI)  ->  UserService  ->  UserRepository  ->  storage
 */
#ifndef USER_SERVICE_H
#define USER_SERVICE_H

#include "user.h"
#include "user_repository.h"


/* Result vocabulary for the service layer. Distinct from RepoStatus so that
 * validation failures are not confused with storage failures. */
typedef enum {
    SERVICE_OK,
    SERVICE_INVALID_NAME,
    SERVICE_INVALID_EMAIL,
    SERVICE_INVALID_PASSWORD,
    SERVICE_NOT_FOUND,
    SERVICE_CONFLICT,
    SERVICE_UNAUTHORIZED,
    SERVICE_ERROR
} ServiceStatus;


/* Holds only a borrowed reference to a repository (constructor injection)*/
typedef struct {
    const UserRepository *repository;
} UserService;

/* Wire a service to any repository implementation. */
void user_service_init(UserService *service, const UserRepository *repository);

/* Validate then persist a new user. On SERVICE_OK, `created` holds the row
 * with its assigned id. `password` is the plaintext password; it is hashed
 * at the storage layer (pgcrypto) and never kept in memory as `created`. */
ServiceStatus user_service_register(
    const UserService *service,
    const char *name, const char *email, const char *password,
    User *created
);

/* Verify email + plaintext password against the stored hash. On SERVICE_OK,
 * `out` holds the authenticated user (password_hash cleared before return).
 * Returns SERVICE_UNAUTHORIZED for either an unknown email or a wrong
 * password (deliberately not distinguished, to avoid user enumeration). */
ServiceStatus user_service_login(
    const UserService *service,
    const char *email, const char *password,
    User *out
);

/* Fetch a single user by id. */
ServiceStatus user_service_get(const UserService *service, long id, User *out);
 
/* Fetch all users into `out` (caller initialises and later frees it). */
ServiceStatus user_service_list(const UserService *service, UserList *out);

/* Validate then update the user identified by `id`. */
ServiceStatus user_service_update(
    const UserService *service, long id,
    const char *name, const char *email
);

/* Remove the user identified by `id`. */
ServiceStatus user_service_remove(const UserService *service, long id);
 
/* Human-readable label for a status, for logging/UI. Never returns NULL. */
const char *service_status_text(ServiceStatus status);
 
#endif /* USER_SERVICE_H */
