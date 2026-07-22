/* user_service.c - Business rules and orchestration for users. */
#include "user_service.h"
 
#include <stdio.h>
#include <string.h>


/* ---- validation (business rules) --------------------------------------- */
 
static int is_blank(const char *text)
{
    if (text == NULL) {
        return 1;
    }
    for (const char *c = text; *c != '\0'; c++)
    {
        if (*c != ' ' && *c != '\t')
        {
            return 0;
        }
    }
    return 1;
}
 
/* A deliberately minimal email check: exactly the rule the domain cares about
 * (has a name part, an '@', and a dot in the domain part). */
static int looks_like_email(const char *text)
{
    if (text == NULL)
    {
        return 0;
    }
 
    const char *at = strchr(text, '@');
    if (at == NULL || at == text)
    {
        return 0;               /* missing '@' or empty local part */
    }
 
    const char *dot = strchr(at, '.');
    return dot != NULL && dot[1] != '\0';
}
 
static ServiceStatus validate(const char *name, const char *email)
{
    if (is_blank(name) || strlen(name) >= USER_NAME_MAX) {
        return SERVICE_INVALID_NAME;
    }
    if (!looks_like_email(email) || strlen(email) >= USER_EMAIL_MAX) {
        return SERVICE_INVALID_EMAIL;
    }
    return SERVICE_OK;
}

/* Minimal password policy: non-empty, fits the column, and long enough to
 * not be trivially guessable. */
static int is_valid_password(const char *password)
{
    if (password == NULL) {
        return 0;
    }
    size_t len = strlen(password);
    return len >= 8 && len < USER_PASSWORD_MAX;
}
 
/* Translate a storage-layer status into a service-layer status. */
static ServiceStatus from_repo_status(RepoStatus status)
{
    switch (status) {
        case REPO_OK:        return SERVICE_OK;
        case REPO_NOT_FOUND: return SERVICE_NOT_FOUND;
        case REPO_CONFLICT:  return SERVICE_CONFLICT;
        case REPO_ERROR:     return SERVICE_ERROR;
    }
    return SERVICE_ERROR;
}
 
/* ---- public API --------------------------------------------------------- */
 
void user_service_init(UserService *service, const UserRepository *repository){
    service->repository = repository;
}
 
ServiceStatus user_service_register(
    const UserService *service,
    const char *name, const char *email, const char *password, const char *rfc,
    User *created
)
{
    ServiceStatus invalid = validate(name, email);
    if (invalid != SERVICE_OK)
    {
        return invalid;
    }
    if (!is_valid_password(password))
    {
        return SERVICE_INVALID_PASSWORD;
    }

    User input = { .id = 0 };
    snprintf(input.name, sizeof(input.name), "%s", name);
    snprintf(input.email, sizeof(input.email), "%s", email);
    snprintf(input.rfc, sizeof(input.rfc), "%s", rfc);
    /* Carried as plaintext only long enough for the repository to hand it to
     * pgcrypto's crypt(); the returned `created` row holds the real hash. */
    snprintf(input.password_hash, sizeof(input.password_hash), "%s", password);
    snprintf(input.role, sizeof(input.role), "%s", "customer");

    return from_repo_status(repo_create(service->repository, &input, created));
}

ServiceStatus user_service_login(
    const UserService *service,
    const char *email, const char *password,
    User *out
)
{
    if (!looks_like_email(email) || !is_valid_password(password))
    {
        return SERVICE_UNAUTHORIZED;
    }

    RepoStatus status = repo_verify_credentials(service->repository, email, password, out);
    if (status == REPO_NOT_FOUND)
    {
        return SERVICE_UNAUTHORIZED;
    }
    if (status != REPO_OK)
    {
        return from_repo_status(status);
    }

    /* Never let the hash linger in memory beyond this point. */
    memset(out->password_hash, 0, sizeof(out->password_hash));
    return SERVICE_OK;
}

ServiceStatus user_service_get(const UserService *service, long id, User *out)
{
    return from_repo_status(repo_find_by_id(service->repository, id, out));
}
 
ServiceStatus user_service_list(const UserService *service, UserList *out)
{
    return from_repo_status(repo_find_all(service->repository, out));
}
 
ServiceStatus user_service_update(
    const UserService *service, long id,
    const char *name, const char *email)
{
    ServiceStatus invalid = validate(name, email);
    if (invalid != SERVICE_OK)
    {
        return invalid;
    }
 
    User input = { .id = id };
    snprintf(input.name, sizeof(input.name), "%s", name);
    snprintf(input.email, sizeof(input.email), "%s", email);
 
    return from_repo_status(repo_update(service->repository, &input));
}
 
ServiceStatus user_service_remove(const UserService *service, long id)
{
    return from_repo_status(repo_remove(service->repository, id));
}
 
const char *service_status_text(ServiceStatus status)
{
    switch (status) {
        case SERVICE_OK:               return "OK";
        case SERVICE_INVALID_NAME:     return "name must be non-empty";
        case SERVICE_INVALID_EMAIL:    return "email is not valid";
        case SERVICE_INVALID_PASSWORD: return "password must be at least 8 characters";
        case SERVICE_NOT_FOUND:        return "user not found";
        case SERVICE_CONFLICT:         return "email already in use";
        case SERVICE_UNAUTHORIZED:     return "invalid email or password";
        case SERVICE_ERROR:            return "storage error";
    }
    return "unknown status";
}