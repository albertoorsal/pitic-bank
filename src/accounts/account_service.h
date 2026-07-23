/*
 * account_service.h - Application/business layer for accounts.
 *
 * The service enforces domain rules (positive amounts, sufficient funds,
 * account ownership) and orchestrates persistence. It depends on the
 * AccountRepository *abstraction*, not on libpq.
 *
 * Separation of concerns:
 *   presentation (CLI)  ->  AccountService  ->  AccountRepository  ->  storage
 */
#ifndef ACCOUNT_SERVICE_H
#define ACCOUNT_SERVICE_H

#include "account.h"
#include "account_repository.h"

/* Result vocabulary for the service layer. Distinct from RepoStatus so that
 * validation failures are not confused with storage failures. */
typedef enum {
    ACCOUNT_SERVICE_OK,
    ACCOUNT_SERVICE_INVALID_AMOUNT,
    ACCOUNT_SERVICE_INSUFFICIENT_FUNDS,
    ACCOUNT_SERVICE_NOT_FOUND,
    ACCOUNT_SERVICE_FORBIDDEN,
    ACCOUNT_SERVICE_SAME_ACCOUNT,
    ACCOUNT_SERVICE_ERROR
} AccountServiceStatus;

/* Holds only a borrowed reference to a repository (constructor injection). */
typedef struct {
    const AccountRepository *repository;
} AccountService;

/* Wire a service to any repository implementation. */
void account_service_init(AccountService *service, const AccountRepository *repository);

/* Open a new checking account for `user_id`, generating a unique account
 * number. On ACCOUNT_SERVICE_OK, `created` holds the stored row. */
AccountServiceStatus account_service_open(
    const AccountService *service, long user_id, Account *created
);

/* Fetch a single account by id. */
AccountServiceStatus account_service_get(const AccountService *service, long id, Account *out);

/* Fetch every account owned by `user_id`. */
AccountServiceStatus account_service_list_for_user(
    const AccountService *service, long user_id, AccountList *out
);

/* Fetch every account (admin view). */
AccountServiceStatus account_service_list_all(const AccountService *service, AccountList *out);

/* Credit `account_id` with `amount_cents` (must be > 0). Fails with
 * ACCOUNT_SERVICE_FORBIDDEN if `owner_user_id` does not own the account. */
AccountServiceStatus account_service_deposit(
    const AccountService *service, long account_id, long owner_user_id,
    long long amount_cents, Account *out
);

/* Debit `account_id` by `amount_cents` (must be > 0 and <= balance). Fails
 * with ACCOUNT_SERVICE_FORBIDDEN if `owner_user_id` does not own the account. */
AccountServiceStatus account_service_withdraw(
    const AccountService *service, long account_id, long owner_user_id,
    long long amount_cents, Account *out
);

/* Move `amount_cents` from the account numbered `from_account_number` (which
 * must be owned by `owner_user_id`) to the account numbered
 * `to_account_number`. On ACCOUNT_SERVICE_OK, `transfer_id` receives the id
 * of the recorded transfer. */
AccountServiceStatus account_service_transfer(
    const AccountService *service,
    const char *from_account_number, long owner_user_id,
    const char *to_account_number,
    long long amount_cents, long *transfer_id
);

/* Human-readable label for a status, for logging/UI. Never returns NULL. */
const char *account_service_status_text(AccountServiceStatus status);

#endif /* ACCOUNT_SERVICE_H */
