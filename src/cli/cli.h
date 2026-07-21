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

/* Run the interactive menu loop until the user chooses to quit.
 * Returns a process exit code (0 = success). */
int cli_run(const UserService *service);

#endif /* CLI_H */