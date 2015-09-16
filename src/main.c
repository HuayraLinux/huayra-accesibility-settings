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
#include "mate-session.h"
#include "populate-cursors.h"

/* Definitions */

#define _(x) x

#define DPI_LOW_REASONABLE_VALUE 50
#define DPI_HIGH_REASONABLE_VALUE 500

#define DPI_FACTOR_LARGE   1.25
#define DPI_FACTOR_LARGER  1.5
#define DPI_FACTOR_LARGEST 2.0
#define DPI_DEFAULT        96

/* Default accesibility settings */

#define MOBILITY_SCHEMA       "org.mate.applications-at-mobility"
#define MOBILITY_KEY          "exec"
#define MOBILITY_STARTUP_KEY  "startup"

#define VISUAL_SCHEMA         "org.mate.applications-at-visual"
#define VISUAL_KEY            "exec"
#define VISUAL_STARTUP_KEY    "startup"

/* Mouse settings */

static GSettings *mouse_settings = NULL;

#define MOUSE_SCHEMA     "org.mate.peripherals-mouse"
#define KEY_CURSOR_THEME "cursor-theme"
#define KEY_CURSOR_SIZE  "cursor-size"

/* Interface settings */

static GSettings *interface_settings = NULL;

#define INTERFACE_SCHEMA "org.mate.interface"
#define KEY_GTK_THEME    "gtk-theme"
#define KEY_COLOR_SCHEME "gtk-color-scheme"
#define KEY_ICON_THEME   "icon-theme"

static GSettings *marco_settings = NULL;

#define MARCO_SCHEMA     "org.mate.Marco.general"
#define KEY_MARCO_THEME  "theme"

#define HIGH_CONTRAST_THEME  "HighContrast"
#define HIGH_CONTRAST_ICON_THEME "huayra-accesible"
#define HIGH_CONTRAST_MARCO_THEME "HuayraAccesible"

static GSettings *font_settings = NULL;

#define FONT_RENDER_SCHEMA "org.mate.font-rendering"
#define KEY_FONT_DPI       "dpi"

/* Widgets */

static GtkWidget *high_contrast_w = NULL;
static GtkWidget *high_dpi_w = NULL;
static GtkWidget *mouse_theme_w = NULL;

static GtkWidget *on_screen_keyboard_w;
static GtkWidget *speacher_w;

/* Global vars */

static gboolean current_on_screen_keyboard = FALSE;
static gboolean current_speacher = FALSE;

/* callback used to open default browser when URLs got clicked */

static void
open_url (const gchar *url, GtkWidget *parent)
{
	gboolean success = TRUE;
	const gchar *argv[3];
	int i = 0;

	if (!gtk_show_uri (NULL, url,  gtk_get_current_event_time (), NULL)) {
		GtkWidget *dialog = NULL;
		dialog = gtk_message_dialog_new (GTK_WINDOW (parent),
		                                 GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
		                                 GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE,
		                                 "%s", _("Unable to open the browser"));
		gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (dialog),
		                                          "%s", "No methods supported");

		g_signal_connect (dialog, "response",
		                  G_CALLBACK (gtk_widget_destroy), NULL);

		gtk_window_present (GTK_WINDOW (dialog));
	}
}

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

static gboolean
need_at_enabled (void)
{
	return
		(gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(on_screen_keyboard_w)) ||
		 gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(speacher_w)));
}

/* Settings callbacks */

static gboolean
high_contrast_is_selected (void)
{
	gchar *gtk_theme = NULL;
	gboolean high_gtk_theme;

	gtk_theme = g_settings_get_string(interface_settings, KEY_GTK_THEME);
	high_gtk_theme = (g_strcmp0(gtk_theme, HIGH_CONTRAST_THEME) == 0);

	g_free (gtk_theme);

	return high_gtk_theme;
}

static void
high_contrast_checkbutton_toggled (GtkToggleButton *button,
                                   gpointer         user_data)
{
	if (gtk_toggle_button_get_active (button)) {
		g_settings_set_string (interface_settings, KEY_GTK_THEME, HIGH_CONTRAST_THEME);
		g_settings_set_string (interface_settings, KEY_ICON_THEME, HIGH_CONTRAST_ICON_THEME);
		g_settings_set_string (marco_settings, KEY_MARCO_THEME, HIGH_CONTRAST_MARCO_THEME);
	}
	else {
		g_settings_reset (interface_settings, KEY_GTK_THEME);
		g_settings_reset (interface_settings, KEY_ICON_THEME);
		g_settings_reset (marco_settings, KEY_MARCO_THEME);
	}
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

static gboolean
large_print_is_selected (void)
{
	gdouble dpi = g_settings_get_double (font_settings, KEY_FONT_DPI);
	return (dpi > get_dpi_from_x_server());
}

static void
large_print_checkbutton_toggled (GtkToggleButton *button,
                                 gpointer         user_data)
{
	gdouble x_dpi, u_dpi;

	font_settings = g_settings_new (FONT_RENDER_SCHEMA);

	if (gtk_toggle_button_get_active (button)) {
		x_dpi = get_dpi_from_x_server ();
		u_dpi = (double)DPI_FACTOR_LARGER * x_dpi;

		g_settings_set_double (font_settings, KEY_FONT_DPI, u_dpi);
	}
	else {
		g_settings_reset (font_settings, KEY_FONT_DPI);
	}
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
	gtk_tree_model_get(model, &iter, COLUMN_THEME_NAME, &active, -1);

	g_settings_set_string (mouse_settings, KEY_CURSOR_THEME, active);
}

/* Launch keyboard */

static void
on_keyboard_accessibility_activated (GtkButton *button,
                                     gpointer   user_data)
{
	GError *error = NULL;
	gboolean result;

	result = g_spawn_command_line_async ("mate-keyboard-properties --a11y", &error);
	if (G_UNLIKELY (result == FALSE)) {
		g_critical ("Can't launch keyboard %s", error->message);
		g_error_free (error);
	}
}

/* Accessibility */

#define ACCESSIBILITY_KEY       "accessibility"
#define ACCESSIBILITY_SCHEMA    "org.mate.interface"

static void
at_enable (gboolean is_enabled)
{
	GSettings *settings = g_settings_new (ACCESSIBILITY_SCHEMA);
	g_settings_set_boolean (settings, ACCESSIBILITY_KEY, is_enabled);
	g_object_unref (settings);
}

static gboolean
at_is_enable (void)
{
	gboolean is_enabled = FALSE;
	GSettings *settings = g_settings_new (ACCESSIBILITY_SCHEMA);
	is_enabled = g_settings_get_boolean (settings, ACCESSIBILITY_KEY);
	g_object_unref (settings);

	return is_enabled;
}

static gboolean
at_is_active (GtkWidget *widget)
{
	AtkObject *atkobj = NULL;
	atkobj = gtk_widget_get_accessible (widget);
	return GTK_IS_ACCESSIBLE (atkobj);
}

static void
do_suggest_logout_responce (GtkDialog *dialog,
                            gint       response_id,
                            gpointer   user_data)
{
	switch (response_id)
	{
		case GTK_RESPONSE_YES:
			do_logout (NULL);
			break;
		case GTK_RESPONSE_NO:
		default:
			break;
	}
	gtk_widget_destroy (GTK_WIDGET(user_data));
}

static void
do_suggest_logout (GtkWidget *parent)
{
	GtkWidget *dialog;
	dialog = gtk_message_dialog_new (GTK_WINDOW(parent),
	                                 GTK_DIALOG_DESTROY_WITH_PARENT,
	                                 GTK_MESSAGE_INFO,
	                                 GTK_BUTTONS_YES_NO,
	                                 _("¿Desea reiniciar su sesión ahora?"));
 	gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG(dialog), _("Para activar/desactivar el lector de pantalla o el teclado debe cerrar la sesión e iniciar nuevamente."));

	g_signal_connect (dialog, "response",
	                  G_CALLBACK (do_suggest_logout_responce),
	                  parent);

	gtk_widget_show_all(dialog);
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
}

/* */

static void
cursor_combo_box_select_current_theme (GtkWidget *combo)
{
	GtkTreeModel *model = NULL;
	GtkTreeIter iter;
	gchar *theme, *value = NULL;
	gboolean have_found = FALSE;

	theme = g_settings_get_string (mouse_settings, KEY_CURSOR_THEME);

	if (!theme)
		return;

	model = gtk_combo_box_get_model (GTK_COMBO_BOX(combo));
	gtk_tree_model_get_iter_first (GTK_TREE_MODEL(model), &iter);
	do
	{
		gtk_tree_model_get (GTK_TREE_MODEL(model), &iter,
		                    COLUMN_THEME_NAME, &value, -1);

		if (!g_ascii_strncasecmp (value, theme, -1))
			break;
	} while (have_found = gtk_tree_model_iter_next (GTK_TREE_MODEL(model), &iter));

	if (have_found)
		gtk_combo_box_set_active_iter (GTK_COMBO_BOX (combo), &iter);
}

/* */

static void
reset_custom_user_changes (void)
{
	g_settings_reset (font_settings, KEY_FONT_DPI);

	g_settings_reset (interface_settings, KEY_GTK_THEME);
	g_settings_reset (interface_settings, KEY_ICON_THEME);

	g_settings_reset (marco_settings, KEY_MARCO_THEME);

	g_settings_reset (mouse_settings, KEY_CURSOR_THEME);
	g_settings_reset (mouse_settings, KEY_CURSOR_SIZE);

	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (speacher_w), FALSE);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (on_screen_keyboard_w), FALSE);
}

static void
show_accessibility_wiki (GtkWidget *parent)
{
	GNetworkMonitor *nmon;
	GError *error = NULL;
	gboolean result;

	nmon = g_network_monitor_get_default();
	if (g_network_monitor_get_network_available(nmon)) {
		open_url ("http://wiki.huayra.conectarigualdad.gob.ar/index.php/Accesibilidad", parent);
	}
	else {
		result = g_spawn_command_line_async ("huayra-visor-manual articles/a/c/c/Accesibilidad.html", &error);
		if (G_UNLIKELY (result == FALSE)) {
			g_critical ("Can't launch huayra-visor-manual: %s", error->message);
			g_error_free (error);
		}
	}
}

/* */

static void
theme_changed_cb (GSettings *settings, gchar *key, gpointer user_data)
{
	if (g_strcmp0(key, KEY_GTK_THEME) == 0) {
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(high_contrast_w),
			high_contrast_is_selected());
	}
	else if (g_strcmp0(key, KEY_FONT_DPI) == 0) {
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(high_dpi_w),
			large_print_is_selected());
	}
	else if (g_strcmp0(key, KEY_CURSOR_THEME) == 0) {
		cursor_combo_box_select_current_theme (mouse_theme_w);
	}
	else {
		g_critical ("Changed %s key", key);
	}
}

/* */

static void
save_atk_changes (GtkWidget *widget)
{
	GSettings *settings = NULL;
	gboolean new_speacher = FALSE, new_on_screen_keyboard = FALSE;
	gboolean need_at = FALSE, at_enabled = FALSE;

	new_speacher = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (speacher_w));
	settings = g_settings_new (VISUAL_SCHEMA);
	g_settings_set_boolean (settings, VISUAL_STARTUP_KEY, new_speacher);
	g_object_unref (settings);

	new_on_screen_keyboard = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (on_screen_keyboard_w));
	settings = g_settings_new (MOBILITY_SCHEMA);
	g_settings_set_boolean (settings, MOBILITY_STARTUP_KEY, new_on_screen_keyboard);
	g_object_unref (settings);

	need_at = (new_speacher || new_on_screen_keyboard);
	at_enabled = at_is_enable ();

	if (at_enabled != need_at) {
		at_enable (need_at);
		do_suggest_logout (widget);
	}
	else {
		gtk_widget_destroy(GTK_WIDGET(widget));
	}
}

static void
dialog_response_cb (GtkDialog *dialog,
                    gint       response,
                    gpointer   user_data)
{
	switch (response)
	{
		case GTK_RESPONSE_HELP:
			show_accessibility_wiki (GTK_WIDGET(dialog));
			break;
		case GTK_RESPONSE_CANCEL:
			reset_custom_user_changes ();
			break;
		case GTK_RESPONSE_OK:
			save_atk_changes (GTK_WIDGET(dialog));
			break;
		default:
			gtk_widget_destroy(GTK_WIDGET(dialog));
			break;
	}
}

static void
activate (GtkApplication *app,
          gpointer        user_data)
{
	GtkWidget *window;
	GtkWidget *table, *label, *check_button, *combo, *scale, *button;
	GtkCellRenderer *renderer;
	GSettings *settings = NULL;
	guint row = 0;

	/* Settings */

	mouse_settings = g_settings_new (MOUSE_SCHEMA);
	marco_settings = g_settings_new (MARCO_SCHEMA);
	interface_settings = g_settings_new (INTERFACE_SCHEMA);
	font_settings = g_settings_new (FONT_RENDER_SCHEMA);

	/* Window */

	window = gtk_dialog_new ();
	gtk_window_set_application (GTK_WINDOW (window), app);
	gtk_window_set_title (GTK_WINDOW (window), _("Opciones de accesibilidad de Huayra"));
	gtk_window_set_icon_name (GTK_WINDOW (window), "preferences-desktop-accessibility");
	gtk_window_set_default_size (GTK_WINDOW (window), 300, 200);

	gtk_window_set_position (GTK_WINDOW (window), GTK_WIN_POS_CENTER);

	/* Content */

	table = huayra_hig_workarea_table_new ();

	/* Visual */

	huayra_hig_workarea_table_add_section_title (table, &row, _("Accesibilidad visual"));

	check_button = gtk_check_button_new_with_label (_("Realzar contraste en los colores"));
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(check_button),
		high_contrast_is_selected());
	huayra_hig_workarea_table_add_wide_control (table, &row, check_button);
	g_signal_connect (check_button, "toggled",
	                  G_CALLBACK (high_contrast_checkbutton_toggled), NULL);

	high_contrast_w = check_button;

	check_button = gtk_check_button_new_with_label (_("Hacer el texto mas grande y fácil de leer"));
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(check_button),
		large_print_is_selected());
	huayra_hig_workarea_table_add_wide_control (table, &row, check_button);
	g_signal_connect (check_button, "toggled",
	                  G_CALLBACK (large_print_checkbutton_toggled), NULL);

	high_dpi_w = check_button;

	/* Cursor */

	label = gtk_label_new (_("Iconos del ratón"));
	combo = gtk_combo_box_new_with_model (GTK_TREE_MODEL(mouse_settings_themes_populate_store()));
	cursor_combo_box_select_current_theme (combo);
	g_signal_connect (combo, "changed",
	                  G_CALLBACK(icon_cursor_theme_changed), NULL);

	mouse_theme_w = combo;

	renderer = gtk_cell_renderer_pixbuf_new ();
	gtk_cell_layout_pack_start (GTK_CELL_LAYOUT(combo), renderer, TRUE);
	gtk_cell_layout_set_attributes(GTK_CELL_LAYOUT(combo), renderer, "pixbuf", COLUMN_THEME_PIXBUF, NULL);
	renderer = gtk_cell_renderer_text_new();
	gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(combo), renderer, TRUE);
	gtk_cell_layout_set_attributes(GTK_CELL_LAYOUT(combo), renderer, "text", COLUMN_THEME_DISPLAY_NAME, NULL);

	huayra_hig_workarea_table_add_row (table, &row, label, combo);

	label = gtk_label_new (_("Tamaño del cursor"));
	scale = gtk_scale_new_with_range (GTK_ORIENTATION_HORIZONTAL, 16, 128, 2);
	gtk_scale_set_draw_value (GTK_SCALE(scale), FALSE);

	g_settings_bind (mouse_settings, KEY_CURSOR_SIZE,
	                 gtk_range_get_adjustment (GTK_RANGE (scale)), "value",
	                 G_SETTINGS_BIND_DEFAULT);

	huayra_hig_workarea_table_add_row (table, &row, label, scale);

	/* Tools */

	huayra_hig_workarea_table_add_section_title (table, &row, _("Herramientas"));

	/* Screen Speacher. */

	button = gtk_toggle_button_new_with_label (_("Utilizar lector en pantalla"));
	huayra_hig_workarea_table_add_wide_control (table, &row, button);

	settings = g_settings_new (VISUAL_SCHEMA);
	g_settings_set_string (settings, VISUAL_KEY, "orca");
	current_speacher = g_settings_get_boolean (settings, VISUAL_STARTUP_KEY);
	g_object_unref (settings);

	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), current_speacher);
	speacher_w = button;

	/* On Screen Keyboard. */

	button = gtk_toggle_button_new_with_label (_("Utilizar teclado en pantalla"));
	huayra_hig_workarea_table_add_wide_control (table, &row, button);

	settings = g_settings_new (MOBILITY_SCHEMA);
	g_settings_set_string (settings, MOBILITY_KEY, "onboard");
	current_on_screen_keyboard = g_settings_get_boolean (settings, MOBILITY_STARTUP_KEY);
	g_object_unref (settings);

	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), current_on_screen_keyboard);
	on_screen_keyboard_w = button;

	/* Sreen Ruller. */

	button = gtk_button_new_with_label (_("Mostrar regla en pantalla"));
	huayra_hig_workarea_table_add_wide_control (table, &row, button);
	g_signal_connect (button, "clicked",
	                  G_CALLBACK(on_screen_ruler_activated), NULL);

	/* Otras opciones */

	huayra_hig_workarea_table_add_section_title (table, &row, _("Otras opciones"));

	button = gtk_button_new_with_label (_("Accesibilidad del teclado"));
	huayra_hig_workarea_table_add_wide_control (table, &row, button);
	g_signal_connect (button, "clicked",
	                  G_CALLBACK(on_keyboard_accessibility_activated), NULL);

	/* Add table and buttons */

	gtk_box_pack_start (GTK_BOX(gtk_dialog_get_content_area (GTK_DIALOG(window))),
	                    table, FALSE, FALSE, 0);

	gtk_dialog_add_buttons (GTK_DIALOG (window),
	                        _("Revertir cambios"), GTK_RESPONSE_CANCEL,
	                        _("Aceptar"), GTK_RESPONSE_OK,
	                        _("Ayuda"), GTK_RESPONSE_HELP,
	                        NULL);

	/* Callback to external changes. */

	g_signal_connect (font_settings, "changed::"KEY_FONT_DPI,
	                  G_CALLBACK (theme_changed_cb), NULL);
	g_signal_connect (interface_settings, "changed::"KEY_GTK_THEME,
	                  G_CALLBACK (theme_changed_cb), NULL);
	g_signal_connect (interface_settings, "changed::"KEY_ICON_THEME,
	                  G_CALLBACK (theme_changed_cb), NULL);
	g_signal_connect (marco_settings, "changed::"KEY_MARCO_THEME,
	                  G_CALLBACK (theme_changed_cb), NULL);
	g_signal_connect (mouse_settings, "changed::"KEY_CURSOR_THEME,
	                  G_CALLBACK (theme_changed_cb), NULL);

	/* Responses buttons */

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
	g_object_unref (marco_settings);
	g_object_unref (interface_settings);
	g_object_unref (font_settings);

	return status;
}