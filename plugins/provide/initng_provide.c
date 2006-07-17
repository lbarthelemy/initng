/*
 * Initng, a next generation sysvinit replacement.
 * Copyright (C) 2006 Ismael Luceno <ismael.luceno@gmail.com>
 * Copyright (C) 2005 Jimmy Wennlund <jimmy.wennlund@gmail.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
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
#include <string.h>
#include <assert.h>

#include <initng_handler.h>
#include <initng_global.h>
#include <initng_plugin_hook.h>
#include <initng_common.h>
#include <initng_toolbox.h>
#include <initng_static_data_id.h>
#include <initng_static_states.h>

#include <initng_env_variable.h>
#include <initng_service_data_types.h>
#include <initng_active_db.h>

INITNG_PLUGIN_MACRO;

static void remove_virtual_service(const char *name);
static int add_virtual_service(const char *name);
static int virtual_service_set_up(const char *name);
static int dont_try_to_stop_start(active_db_h * service);


s_entry PROVIDE = {
	"provide", STRINGS, NULL,
	"Used by service providers, automagically creates virtual services.",
	NULL
};

/* this service type will a virtual provided get */
stype_h PROVIDED = { "provided", "If a service sets provide, this virtual service is created with that name.", TRUE, &dont_try_to_stop_start, &dont_try_to_stop_start, &dont_try_to_stop_start };

/* num. of providers */
s_entry PCOUNT = { "p_count", INT, &PROVIDED, "Internal variable only", NULL };	/* used to keep track of */

/* THE UP STATE GOT */
a_state_h SOON_PROVIDED = { "SOON_PROVIDED", "The service providing this is starting.", IS_STARTING, NULL, NULL, NULL };
a_state_h PROVIDE_UP = { "PROVIDED", "This service is provided by a parent service.", IS_UP, NULL, NULL, NULL };
a_state_h PROVIDE_DOWN = { "NOT_PROVIDED", "This service is no longer provided by a parent service.", IS_DOWN, NULL, NULL, NULL };


static int dont_try_to_stop_start(active_db_h * service)
{
	D_("You have to stop/start/restart the service providing this!\n");
	return (FALSE);
}

static int virtual_service_set_up(const char *name)
{
	active_db_h *vserv;

	if ((vserv = initng_active_db_find_by_exact_name(name)))
	{
		if (IS_STARTING(vserv))
		{
			initng_common_mark_service(vserv, &PROVIDE_UP);
			return (TRUE);
		}
	}
	return (FALSE);
}

static int add_virtual_service(const char *name)
{
	active_db_h *vserv;


	/* if found */
	if ((vserv = initng_active_db_find_by_exact_name(name)))
	{
		/* make sure its a PROVIDED TYPE, else continue */
		if (vserv->type != &PROVIDED)
		{
			F_("Service name providing is used by another service type\n");
			return (FALSE);
		}

		/* put the service as UP */
		if (!IS_UP(vserv) || !IS_STARTING(vserv))
			initng_common_mark_service(vserv, &SOON_PROVIDED);

		/* increase the counter */
		set_int(&PCOUNT, vserv, get_int(&PCOUNT, vserv) + 1);

		/* return happily */
		return (TRUE);
	}
	else
	{
		/* if not found */
		/* create a new */
		if (!(vserv = initng_active_db_new(name)))
		{
			F_("Failed to create %s\n", name);
			return (FALSE);
		}

		/* set type */
		vserv->type = &PROVIDED;

#ifdef SERVICE_CACHE
		/* make sure it wont try to parse this entry */
		vserv->from_service = &NO_CACHE;
#endif

		/* register it */
		if (!initng_active_db_register(vserv))
		{
			F_("Failed to register %s\n", vserv->name);
			initng_active_db_free(vserv);
			return (FALSE);
		}

		/* put as up */
		initng_common_mark_service(vserv, &SOON_PROVIDED);

		/* set use-count to 1 */
		set_int(&PCOUNT, vserv, 1);

		/* exit happily */
		return (TRUE);
	}
}

static void remove_virtual_service(const char *name)
{
	active_db_h *vserv;
	int i = 0;

	/* find that exact service */
	if (!(vserv = initng_active_db_find_by_exact_name(name)))
	{
		W_("Virtual service %s doesn't exist!\n", name);
		return;
	}

	/* make sure this found is a provided type */
	if (vserv->type != &PROVIDED)
	{
		F_("Provided is not an provided type\n");
		return;
	}

	/* Look for PCOUNTS */
	if (IS_UP(vserv))
	{
		i = get_int(&PCOUNT, vserv) - 1;
		set_int(&PCOUNT, vserv, i);
	}

	/* if no one is used it OR system is stopping */
	if (!i || g.sys_state == STATE_STOPPING)
	{
		/* NOTE: all service marked DOWN will dissapere from active list within a minute or so */
		initng_common_mark_service(vserv, &PROVIDE_DOWN);

		/* TODO: test the following code; initng crashes and free()
		 *       complains about invalid pointer.
		 *    NOTE: probably becouse d_get_int_var with an var name, that cant be free (vhar *) "count".
		 * initng_active_db_free(vserv);
		 */
	}
}

/*
 * This hook is for monitor service changes.
 */
static int service_state(active_db_h * service)
{
	const char *tmp = NULL;
	s_data *itt = NULL;
	char *fixed;

	assert(service);
	assert(service->name);


	/* if servie is starting */
	if (IS_STARTING(service))
	{
		/* never when system is stopping */
		if (g.sys_state == STATE_STOPPING)
			return (TRUE);

		/* get the provide strings */
		while ((tmp = get_next_string(&PROVIDE, service, &itt)))
		{
			/* fix $vars */
			fixed = fix_variables(tmp, service);

			/* add that provide */
			if (!add_virtual_service(fixed))
				return (FALSE);

			fix_free(fixed, tmp);
		}
	}
	else if (IS_UP(service))
	{
		/* never when system is stopping */
		if (g.sys_state == STATE_STOPPING)
			return (TRUE);

		/* get the provide strings */
		while ((tmp = get_next_string(&PROVIDE, service, &itt)))
		{
			/* fix $vars */
			fixed = fix_variables(tmp, service);

			/* add that provide */
			if (!virtual_service_set_up(fixed))
				return (FALSE);

			fix_free(fixed, tmp);
		}
	}
	/* else - its down */
	else
	{
		/* get the provide strings */
		while ((tmp = get_next_string(&PROVIDE, service, &itt)))
		{
			/* fix $vars */
			fixed = fix_variables(tmp, service);

			/* remove that provide */
			remove_virtual_service(fixed);

			fix_free(fixed, tmp);
		}
	}

	/* always return happily */
	return (TRUE);
}


#ifdef EXTRA_SURE
static void system_stopping(h_sys_state state)
{
	active_db_h *current = NULL;

	/* only do this if system is stopping */
	if (state != STATE_STOPPING)
		return;

	/* find all netdev types and stop them */
	while_active_db(current)
	{
		if (current->type == &PROVIDED)
		{
			/* mark service down */
			initng_common_mark_service(current, &PROVIDE_DOWN);
		}
	}
}
#endif


int module_init(int api_version)
{
	D_("module_init();\n");

	if (api_version != API_VERSION)
	{
		F_("This module is compiled for api_version %i version and initng is compiled on %i version, won't load this module!\n", API_VERSION, api_version);
		return (FALSE);
	}

	initng_service_type_register(&PROVIDED);
	initng_active_state_register(&SOON_PROVIDED);
	initng_active_state_register(&PROVIDE_UP);
	initng_active_state_register(&PROVIDE_DOWN);
	initng_service_data_type_register(&PROVIDE);
	initng_service_data_type_register(&PCOUNT);
#ifdef EXTRA_SURE
	initng_plugin_hook_register(&g.SWATCHERS, 80, &system_stopping);
#endif
	initng_plugin_hook_register(&g.IS_CHANGE, 50, &service_state);

	return (TRUE);
}

void module_unload(void)
{
	D_("module_unload();\n");
	initng_plugin_hook_unregister(&g.IS_CHANGE, &service_state);
	initng_service_data_type_unregister(&PROVIDE);
	initng_service_data_type_unregister(&PCOUNT);
	initng_active_state_unregister(&SOON_PROVIDED);
	initng_active_state_unregister(&PROVIDE_UP);
	initng_active_state_unregister(&PROVIDE_DOWN);
	initng_service_type_unregister(&PROVIDED);
#ifdef EXTRA_SURE
	initng_plugin_hook_unregister(&g.SWATCHERS, &system_stopping);
#endif
}
