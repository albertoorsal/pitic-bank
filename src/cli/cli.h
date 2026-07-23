/*
 * cli.h - Presentation layer: an interactive text menu.
 *
 * The CLI's only job is input/output. It never talks to the database; it only
 * calls the UserService. Swap this for an HTTP handler or a GUI without
 * touching any business or storage code.
 */
#ifndef CLI_H
#define CLI_H

#include "user/user_service.h"
#include "accounts/account_service.h"
#include "transfers/transfer_service.h"

/* Every service the presentation layer needs, bundled for convenience. The
 * CLI only ever reads through this; it never touches storage directly. */
typedef struct {
    const UserService *users;
    const AccountService *accounts;
    const TransferService *transfers;
} AppContext;

/* Run the interactive menu loop until the user chooses to quit.
 * Returns a process exit code (0 = success). */
int cli_run(const AppContext *app);

#endif /* CLI_H */