/*
 * Initng, a next generation sysvinit replacement.
 * Copyright (C) 2006 Jimmy Wennlund <jimmy.wennlund@gmail.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include <initng.h>

#include <stdio.h>
#include <stdlib.h>
#include <sys/poll.h>
#include <fcntl.h>
#include <string.h>
#include <time.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <sys/socket.h>
#include <string.h>
#include <assert.h>

static int module_init(void);
static void module_unload(void);

const struct initng_module initng_module = {
	.api_version = API_VERSION,
	.deps = { "runlevel", NULL },
	.init = &module_init,
	.unload = &module_unload
};

static int cmd_stop_unneeded(char *arg);

s_command STOP_UNNEEDED = {
	.id = 'y',
	.long_id = "stop_unneeded",
	.com_type = INT_COMMAND,
	.opt_visible = STANDARD_COMMAND,
	.opt_type = NO_OPT,
	.u = {(void *)&cmd_stop_unneeded},
	.description = "Stop all services, not in runlevel"
};

static int cmd_stop_unneeded(char *arg)
{
	int needed = FALSE;
	active_db_h *A, *As = NULL;
	active_db_h *B = NULL;
	stype_h *TYPE_RUNLEVEL = initng_service_type_get_by_name("runlevel");

	S_;

	/* walk through all and check */
	while_active_db_safe(A, As) {
		if (A->type == TYPE_RUNLEVEL)
			continue;

		/* reset variables */
		needed = FALSE;
		B = NULL;

		while_active_db(B) {
			/* don't check ourself */
			if (A == B)
				continue;

			/* if B needs A */
			if (initng_depend(B, A) == TRUE) {
				needed = TRUE;
				break;
			}
		}

		/* if there was no needed this service, stop it */
		if (needed == FALSE)
			initng_handler_stop_service(A);
	}

	return TRUE;
}

int module_init(void)
{
	initng_command_register(&STOP_UNNEEDED);
	return TRUE;
}

void module_unload(void)
{
	initng_command_unregister(&STOP_UNNEEDED);
}
