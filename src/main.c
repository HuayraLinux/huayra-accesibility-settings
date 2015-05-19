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

#include <gtk/gtk.h>

#include "huayra-hig.h"
#include "populate-cursors.h"

#define _(x) x

#define DPI_LOW_REASONABLE_VALUE 50
#define DPI_HIGH_REASONABLE_VALUE 500

#define DPI_FACTOR_LARGE   1.25
#define DPI_FACTOR_LARGER  1.5
#define DPI_FACTOR_LARGEST 2.0
#define DPI_DEFAULT        96

/* Dconf settings */

#define MOUSE_SCHEMA     "org.mate.peripherals-mouse"
#define KEY_CURSOR_THEME "cursor-theme"
#define KEY_CURSOR_SIZE  "cursor-size"

#define INTERFACE_SCHEMA "org.mate.interface"
#define KEY_GTK_THEME    "gtk-theme"
#define KEY_COLOR_SCHEME "gtk-color-scheme"
#define KEY_ICON_THEME   "icon-theme"

#define MARCO_SCHEMA     "org.mate.Marco"
#define KEY_MARCO_THEME  "theme"

#define HIGH_CONTRAST_THEME  "HighContrast"

#define FONT_RENDER_SCHEMA "org.mate.font-rendering"
#define KEY_FONT_DPI       "dpi"

static GSettings *mouse_settings = NULL;

/* */

static gboolean
dbus_interface_name_is_running (const gchar *name)
{
	GDBusConnection *gconnection;
	GError *gerror = NULL, *error = NULL;
	GVariant *v;
	GVariantIter *iter;
	const gchar *str = NULL;
	gboolean result = FALSE;

	gconnection = g_bus_get_sync (G_BUS_TYPE_SESSION, NULL, &gerror);
	if (gconnection == NULL) {
		g_message ("Failed to get session bus: %s", gerror->message);
		g_error_free (gerror);

		return FALSE;
	}

	v = g_dbus_connection_call_sync (gconnection,
	                                 "org.freedesktop.DBus",
	                                 "/org/freedesktop/DBus",
	                                 "org.freedesktop.DBus",
	                                 "ListNames",
	                                 NULL,
	                                 G_VARIANT_TYPE ("(as)"),
	                                 G_DBUS_CALL_FLAGS_NONE,
	                                 -1,
	                                 NULL,
	                                 &error);

	if (error) {
		g_critical ("Could not get a list of names registered on the session bus, %s",
		            error ? error->message : "no error given");
		g_clear_error (&error);
		return FALSE;
	}

	g_variant_get (v, "(as)", &iter);
	while (g_variant_iter_loop (iter, "&s", &str)) {
		if (g_strcmp0(str, name) == 0) {
			result = TRUE;
			break;
		}
	}

	g_variant_iter_free (iter);
	g_variant_unref (v);

	return result;
}

static gboolean
process_is_running (const char * name)
{
	int num_processes;
	char * command = g_strdup_printf ("pidof %s | wc -l", name);
	FILE *fp = popen(command, "r");

	fscanf(fp, "%d", &num_processes);
	pclose(fp);

	g_free (command);

	if (num_processes > 0) {
		return TRUE;
	}
	else {
		return FALSE;
	}
}

/* Settings callbacks */

static void
high_contrast_checkbutton_toggled (GtkToggleButton *button,
                                   gpointer         user_data)
{
	GSettings *interface_settings = NULL;
	GSettings *marco_settings = NULL;

	interface_settings = g_settings_new (INTERFACE_SCHEMA);
	marco_settings = g_settings_new (MARCO_SCHEMA);

	if (gtk_toggle_button_get_active (button)) {
		g_settings_set_string (interface_settings, KEY_GTK_THEME, HIGH_CONTRAST_THEME);
		g_settings_set_string (interface_settings, KEY_ICON_THEME, HIGH_CONTRAST_THEME);
	}
	else {
		g_settings_reset (interface_settings, KEY_GTK_THEME);
		g_settings_reset (interface_settings, KEY_ICON_THEME);
		g_settings_reset (marco_settings, KEY_MARCO_THEME);
	}
	g_object_unref (interface_settings);
	g_object_unref (marco_settings);
}

static double
dpi_from_pixels_and_mm (int pixels,
                        int mm)
{
	double dpi;

	if (mm >= 1) {
		dpi = pixels / (mm / 25.4);
	}
	else {
		dpi = 0;
	}

	return dpi;
}

static double
get_dpi_from_x_server (void)
{
	GdkScreen *screen;
	double dpi, width_dpi, height_dpi;

	screen = gdk_screen_get_default ();
	if (screen != NULL) {
		width_dpi = dpi_from_pixels_and_mm (gdk_screen_get_width (screen),
		                                    gdk_screen_get_width_mm (screen));
		height_dpi = dpi_from_pixels_and_mm (gdk_screen_get_height (screen),
		                                     gdk_screen_get_height_mm (screen));
		if (width_dpi < DPI_LOW_REASONABLE_VALUE
		    || width_dpi > DPI_HIGH_REASONABLE_VALUE
		    || height_dpi < DPI_LOW_REASONABLE_VALUE
		    || height_dpi > DPI_HIGH_REASONABLE_VALUE) {
			dpi = DPI_DEFAULT;
		}
		else {
			dpi = (width_dpi + height_dpi) / 2.0;
		}
	}
	else {
		dpi = DPI_DEFAULT;
	}
	return dpi;
}

static void
large_print_checkbutton_toggled (GtkToggleButton *button,
                                 gpointer         user_data)
{
	GSettings *settings;
	gdouble x_dpi, u_dpi;

	settings = g_settings_new (FONT_RENDER_SCHEMA);

	if (gtk_toggle_button_get_active (button)) {
		x_dpi = get_dpi_from_x_server ();
		u_dpi = (double)DPI_FACTOR_LARGER * x_dpi;

		g_settings_set_double (settings, KEY_FONT_DPI, u_dpi);
	}
	else {
		g_settings_reset (settings, KEY_FONT_DPI);
	}
	g_object_unref (settings);
}

static void
icon_cursor_theme_changed (GtkComboBox *combo,
                           gpointer     user_data)
{
	GtkTreeModel *model;
	GtkTreeIter iter;
	gchar *active;

	gtk_combo_box_get_active_iter(combo, &iter);

	model = gtk_combo_box_get_model(combo);
	gtk_tree_model_get(model, &iter, 2, &active, -1);

	g_settings_set_string (mouse_settings, KEY_CURSOR_THEME, active);
}

static void
on_screen_keyboard_activated (GtkButton *button,
                              gpointer   user_data)
{
	GError *error = NULL;
	gboolean result;

	/* TODO: Get default app*/

	if (dbus_interface_name_is_running ("org.florence.Keyboard"))
		return;

	result = g_spawn_command_line_async ("florence", &error);
	if (G_UNLIKELY (result == FALSE)) {
		g_critical ("Can't launch keyboard %s", error->message);
		g_error_free (error);
	}
}

static void
on_screen_ruler_activated (GtkButton *button,
                           gpointer   user_data)
{
	GError *error = NULL;
	gboolean result;

	if (process_is_running ("ruby /usr/bin/screenruler"))
		return;

	/* TODO: Set a dconf setting. */
	result = g_spawn_command_line_async ("screenruler", &error);
	if (G_UNLIKELY (result == FALSE)) {
		g_critical ("Can't launch screen ruler: %s", error->message);
		g_error_free (error);
	}
	g_spawn_command_line_async ("screenruler", &error);
}


/* */

static void
dialog_response_cb (GtkDialog *dialog,
                    gint       response,
                    gpointer   user_data)
{
	gtk_widget_destroy(GTK_WIDGET(dialog));
}

static void
activate (GtkApplication *app,
          gpointer        user_data)
{
	GtkWidget *window;
	GtkWidget *table, *label, *check_button, *combo, *scale, *button;
	GtkCellRenderer *renderer;
	guint row = 0;

	/* Settings */

	mouse_settings = g_settings_new (MOUSE_SCHEMA);

	/* Window */

	window = gtk_dialog_new ();
	gtk_window_set_application (GTK_WINDOW (window), app);
	gtk_window_set_title (GTK_WINDOW (window), _("Huayra Accessibility Settings"));
	gtk_window_set_icon_name (GTK_WINDOW (window), "preferences-desktop-accessibility");
	gtk_window_set_default_size (GTK_WINDOW (window), 300, 200);

	gtk_window_set_position (GTK_WINDOW (window), GTK_WIN_POS_CENTER);

	/* Content */

	table = huayra_hig_workarea_table_new ();

	/* Visual */

	huayra_hig_workarea_table_add_section_title (table, &row, _("Visual accessibility"));

	check_button = gtk_check_button_new_with_label (_("Realzar contraste en los colores"));
	huayra_hig_workarea_table_add_wide_control (table, &row, check_button);
	g_signal_connect (check_button, "toggled",
	                  G_CALLBACK (high_contrast_checkbutton_toggled), NULL);

	check_button = gtk_check_button_new_with_label (_("Hacer el texto mas grande y facil de leer"));
	huayra_hig_workarea_table_add_wide_control (table, &row, check_button);
	g_signal_connect (check_button, "toggled",
	                  G_CALLBACK (large_print_checkbutton_toggled), NULL);

	/* Cursor */

	label = gtk_label_new (_("Iconos del raton"));
	combo = gtk_combo_box_new_with_model (GTK_TREE_MODEL(mouse_settings_themes_populate_store()));
	g_signal_connect (combo, "changed",
	                  G_CALLBACK(icon_cursor_theme_changed), NULL);

	renderer = gtk_cell_renderer_pixbuf_new ();
	gtk_cell_layout_pack_start (GTK_CELL_LAYOUT(combo), renderer, TRUE);
	gtk_cell_layout_set_attributes(GTK_CELL_LAYOUT(combo), renderer, "pixbuf", 0, NULL);
	renderer = gtk_cell_renderer_text_new();
	gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(combo), renderer, TRUE);
	gtk_cell_layout_set_attributes(GTK_CELL_LAYOUT(combo), renderer, "text", 3, NULL);

	huayra_hig_workarea_table_add_row (table, &row, label, combo);

	label = gtk_label_new (_("Tamaño del cursor"));
	scale = gtk_scale_new_with_range (GTK_ORIENTATION_HORIZONTAL, 0, 100, 2);
	gtk_scale_set_draw_value (GTK_SCALE(scale), FALSE);

	g_settings_bind (mouse_settings, KEY_CURSOR_SIZE,
	                 gtk_range_get_adjustment (GTK_RANGE (scale)), "value",
	                 G_SETTINGS_BIND_DEFAULT);

	huayra_hig_workarea_table_add_row (table, &row, label, scale);

	/* Tools */

	huayra_hig_workarea_table_add_section_title (table, &row, _("Herramientas"));

	button = gtk_button_new_with_label (_("Utilizar teclado en pantalla"));
	huayra_hig_workarea_table_add_wide_control (table, &row, button);
	g_signal_connect (button, "clicked",
	                  G_CALLBACK(on_screen_keyboard_activated), NULL);

	button = gtk_button_new_with_label (_("Utilizar regla en pantalla"));
	huayra_hig_workarea_table_add_wide_control (table, &row, button);
	g_signal_connect (button, "clicked",
	                  G_CALLBACK(on_screen_ruler_activated), NULL);

	gtk_box_pack_start (GTK_BOX(gtk_dialog_get_content_area (GTK_DIALOG(window))),
	                    table, FALSE, FALSE, 0);

	gtk_dialog_add_buttons (GTK_DIALOG (window),
	                        _("Aceptar"), GTK_RESPONSE_OK,
	                        NULL);

	g_signal_connect (window, "response",
	                  G_CALLBACK (dialog_response_cb), NULL);

	gtk_widget_show_all (window);
}

int
main (int    argc,
      char **argv)
{
	GtkApplication *app;
	int status;

	app = gtk_application_new ("org.accessibility.huayra", G_APPLICATION_FLAGS_NONE);
	g_signal_connect (app, "activate", G_CALLBACK (activate), NULL);
	status = g_application_run (G_APPLICATION (app), argc, argv);
	g_object_unref (app);

	/* Free resources */
	g_object_unref (mouse_settings);

	return status;
}