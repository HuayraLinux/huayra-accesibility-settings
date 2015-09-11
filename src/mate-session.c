/*************************************************************************/
/* Copyright (C) 2015 matias <mati86dl@gmail.com>                        */
/*                                                                       */
/* This program is free software: you can redistribute it and/or modify  */
/* it under the terms of the GNU General Public License as published by  */
/* the Free Software Foundation, either version 3 of the License, or     */
/* (at your option) any later version.                                   */
/*                                                                       */
/* This program is distributed in the hope that it will be useful,       */
/* but WITHOUT ANY WARRANTY; without even the implied warranty of        */
/* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         */
/* GNU General Public License for more details.                          */
/*                                                                       */
/* You should have received a copy of the GNU General Public License     */
/* along with this program.  If not, see <http://www.gnu.org/licenses/>. */
/*************************************************************************/

/* get_session_bus(), get_sm_proxy(), and do_logout() are all
 * based on code from mate-session-save.c from mate-session.
 */

#define GSM_SERVICE_DBUS   "org.mate.SessionManager"
#define GSM_PATH_DBUS      "/org/mate/SessionManager"
#define GSM_INTERFACE_DBUS "org.mate.SessionManager"

#include <dbus/dbus-glib.h>
#include <dbus/dbus-glib-lowlevel.h>

#include "mate-session.h"

static DBusGConnection *
get_session_bus (void)
{
        DBusGConnection *bus;
        GError *error = NULL;

        bus = dbus_g_bus_get (DBUS_BUS_SESSION, &error);

        if (bus == NULL) {
                g_warning ("Couldn't connect to session bus: %s", error->message);
                g_error_free (error);
        }

        return bus;
}

static DBusGProxy *
get_sm_proxy (void)
{
        DBusGConnection *connection;
        DBusGProxy      *sm_proxy;

        if (!(connection = get_session_bus ()))
		return NULL;

        sm_proxy = dbus_g_proxy_new_for_name (connection,
					      GSM_SERVICE_DBUS,
					      GSM_PATH_DBUS,
					      GSM_INTERFACE_DBUS);

        return sm_proxy;
}

gboolean
do_logout (GError **err)
{
        DBusGProxy *sm_proxy;
        GError     *error;
        gboolean    res;

        sm_proxy = get_sm_proxy ();
        if (sm_proxy == NULL)
		return FALSE;

        res = dbus_g_proxy_call (sm_proxy,
                                 "Logout",
                                 &error,
                                 G_TYPE_UINT, 0,   /* '0' means 'log out normally' */
                                 G_TYPE_INVALID,
                                 G_TYPE_INVALID);

        if (sm_proxy)
                g_object_unref (sm_proxy);

	return res;
}
