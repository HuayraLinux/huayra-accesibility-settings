/*
 *  Copyright (c) 2008-2011 Nick Schermer <nick@xfce.org>
 *  Copyright (c) 2008      Jannis Pohlmann <jannis@xfce.org>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include <gtk/gtk.h>

enum
{
    COLUMN_THEME_PIXBUF,
    COLUMN_THEME_PATH,
    COLUMN_THEME_NAME,
    COLUMN_THEME_DISPLAY_NAME,
    COLUMN_THEME_COMMENT,
    N_THEME_COLUMNS
};

GtkListStore *
mouse_settings_themes_populate_store (void);
