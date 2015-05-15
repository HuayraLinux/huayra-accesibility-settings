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

/*
 * This file is part of xfce4-setting code.
 * Based on: http://git.xfce.org/xfce/xfce4-settings/tree/dialogs/mouse-settings/main.c
 */

#include <X11/Xcursor/Xcursor.h>
#include <libxfce4util/libxfce4util.h>
#include <math.h>

#include "populate-cursors.h"

/* icon names for the preview widget */
static const gchar *preview_names[] = {
    "left_ptr",            "left_ptr_watch",    "watch",             "hand2",
    "question_arrow",      "sb_h_double_arrow", "sb_v_double_arrow", "bottom_left_corner",
    "bottom_right_corner", "fleur",             "pirate",            "cross",
    "X_cursor",            "right_ptr",         "right_side",        "right_tee",
    "sb_right_arrow",      "sb_right_tee",      "base_arrow_down",   "base_arrow_up",
    "bottom_side",         "bottom_tee",        "center_ptr",        "circle",
    "dot",                 "dot_box_mask",      "dot_box_mask",      "double_arrow",
    "draped_box",          "left_side",         "left_tee",          "ll_angle",
    "top_side",            "top_tee"
};

enum
{
    COLUMN_THEME_PIXBUF,
    COLUMN_THEME_PATH,
    COLUMN_THEME_NAME,
    COLUMN_THEME_DISPLAY_NAME,
    COLUMN_THEME_COMMENT,
    N_THEME_COLUMNS
};

#define PREVIEW_ROWS    (3)
#define PREVIEW_COLUMNS (6)
#define PREVIEW_SIZE    (24)
#define PREVIEW_SPACING (2)

static GdkPixbuf *
mouse_settings_themes_pixbuf_from_filename (const gchar *filename,
                                            guint        size)
{
    XcursorImage *image;
    GdkPixbuf    *scaled, *pixbuf = NULL;
    gsize         bsize;
    guchar       *buffer, *p, tmp;
    gdouble       wratio, hratio;
    gint          dest_width, dest_height;

    /* load the image */
    image = XcursorFilenameLoadImage (filename, size);
    if (G_LIKELY (image))
    {
        /* buffer size */
        bsize = image->width * image->height * 4;

        /* allocate buffer */
        buffer = g_malloc (bsize);

        /* copy pixel data to buffer */
        memcpy (buffer, image->pixels, bsize);

        /* swap bits */
        for (p = buffer; p < buffer + bsize; p += 4)
        {
            tmp = p[0];
            p[0] = p[2];
            p[2] = tmp;
        }

        /* create pixbuf */
        pixbuf = gdk_pixbuf_new_from_data (buffer, GDK_COLORSPACE_RGB, TRUE,
                                           8, image->width, image->height,
                                           4 * image->width,
                                           (GdkPixbufDestroyNotify) g_free, NULL);

        /* don't leak when creating the pixbuf failed */
        if (G_UNLIKELY (pixbuf == NULL))
            g_free (buffer);

        /* scale pixbuf if needed */
        if (pixbuf && (image->height > size || image->width > size))
        {
            /* calculate the ratio */
            wratio = (gdouble) image->width / (gdouble) size;
            hratio = (gdouble) image->height / (gdouble) size;

            /* init */
            dest_width = dest_height = size;

            /* set dest size */
            /*if (hratio > wratio)
                dest_width  = rint (image->width / hratio);
            else
                dest_height = rint (image->height / wratio);*/
            dest_height = 16;
            dest_width = 16;

            /* scale pixbuf */
            scaled = gdk_pixbuf_scale_simple (pixbuf, MAX (dest_width, 1), MAX (dest_height, 1), GDK_INTERP_BILINEAR);

            /* release and set scaled pixbuf */
            g_object_unref (G_OBJECT (pixbuf));
            pixbuf = scaled;
        }

        /* cleanup */
        XcursorImageDestroy (image);
    }

    return pixbuf;
}



static GdkPixbuf *
mouse_settings_themes_preview_icon (const gchar *path)
{
    GdkPixbuf *pixbuf = NULL;
    gchar     *filename;

    /* we only try the normal cursor, it is (most likely) always there */
    filename = g_build_filename (path, "left_ptr", NULL);

    /* try to load the pixbuf */
    pixbuf = mouse_settings_themes_pixbuf_from_filename (filename, PREVIEW_SIZE);

    /* cleanup */
    g_free (filename);

    return pixbuf;
}



static void
mouse_settings_themes_preview_image (const gchar *path,
                                     GtkImage    *image)
{
    GdkPixbuf *pixbuf;
    GdkPixbuf *preview;
    guint      i, position;
    gchar     *filename;
    gint       dest_x, dest_y;

    /* create an empty preview image */
    preview = gdk_pixbuf_new (GDK_COLORSPACE_RGB, TRUE, 8,
                              (PREVIEW_SIZE + PREVIEW_SPACING) * PREVIEW_COLUMNS - PREVIEW_SPACING,
                              (PREVIEW_SIZE + PREVIEW_SPACING) * PREVIEW_ROWS - PREVIEW_SPACING);

    if (G_LIKELY (preview))
    {
        /* make the pixbuf transparent */
        gdk_pixbuf_fill (preview, 0x00000000);

        for (i = 0, position = 0; i < G_N_ELEMENTS (preview_names); i++)
        {
            /* create cursor filename and try to load the pixbuf */
            filename = g_build_filename (path, preview_names[i], NULL);
            pixbuf = mouse_settings_themes_pixbuf_from_filename (filename, PREVIEW_SIZE);
            g_free (filename);

            if (G_LIKELY (pixbuf))
            {
                /* calculate the icon position */
                dest_x = (position % PREVIEW_COLUMNS) * (PREVIEW_SIZE + PREVIEW_SPACING);
                dest_y = (position / PREVIEW_COLUMNS) * (PREVIEW_SIZE + PREVIEW_SPACING);

                /* render it in the preview */
                gdk_pixbuf_scale (pixbuf, preview, dest_x, dest_y,
                                  gdk_pixbuf_get_width (pixbuf),
                                  gdk_pixbuf_get_height (pixbuf),
                                  dest_x, dest_y,
                                  1.00, 1.00, GDK_INTERP_BILINEAR);


                /* release the pixbuf */
                g_object_unref (G_OBJECT (pixbuf));

                /* break if we've added enough icons */
                if (++position >= PREVIEW_ROWS * PREVIEW_COLUMNS)
                    break;
            }
        }

        /* set the image */
        gtk_image_set_from_pixbuf (GTK_IMAGE (image), preview);

        /* release the pixbuf */
        g_object_unref (G_OBJECT (preview));
    }
    else
    {
        /* clear the image */
        gtk_image_clear (GTK_IMAGE (image));
    }
}

static gint
mouse_settings_themes_sort_func (GtkTreeModel *model,
                                 GtkTreeIter  *a,
                                 GtkTreeIter  *b,
                                 gpointer      user_data)
{
    gchar *name_a, *name_b;
    gint   retval;

    /* get the names from the model */
    gtk_tree_model_get (model, a, COLUMN_THEME_DISPLAY_NAME, &name_a, -1);
    gtk_tree_model_get (model, b, COLUMN_THEME_DISPLAY_NAME, &name_b, -1);

    /* make sure the names are not null */
    if (G_UNLIKELY (name_a == NULL))
        name_a = g_strdup ("");
    if (G_UNLIKELY (name_b == NULL))
        name_b = g_strdup ("");

    /* sort the names but keep Default on top */
    if (g_utf8_collate (name_a, _("Default")) == 0)
        retval = -1;
    else if (g_utf8_collate (name_b, _("Default")) == 0)
        retval = 1;
    else
        retval = g_utf8_collate (name_a, name_b);

    /* cleanup */
    g_free (name_a);
    g_free (name_b);

    return retval;
}

GtkListStore *
mouse_settings_themes_populate_store (void)
{
    const gchar        *path;
    gchar             **basedirs;
    gint                i;
    gchar              *homedir;
    GDir               *dir;
    const gchar        *theme;
    gchar              *filename;
    gchar              *index_file;
    XfceRc             *rc;
    const gchar        *name;
    const gchar        *comment;
    GtkTreeIter         iter;
    gint                position = 0;
    GdkPixbuf          *pixbuf;
    gchar              *active_theme = NULL;
    GtkTreePath        *active_path = NULL;
    GtkListStore       *store;
    GtkCellRenderer    *renderer;
    GtkTreeViewColumn  *column;
    GObject            *treeview;
    GtkTreeSelection   *selection;
    gchar              *comment_escaped;

    /* get the cursor paths */
#if XCURSOR_LIB_MAJOR == 1 && XCURSOR_LIB_MINOR < 1
    path = "~/.icons:/usr/share/icons:/usr/share/pixmaps:/usr/X11R6/lib/X11/icons";
#else
    path = XcursorLibraryPath ();
#endif

    /* split the paths */
    basedirs = g_strsplit (path, ":", -1);

    /* get the active theme */
    //active_theme = xfconf_channel_get_string (xsettings_channel, "/Gtk/CursorThemeName", "default");

    /* create the store */
    store = gtk_list_store_new (N_THEME_COLUMNS, GDK_TYPE_PIXBUF, G_TYPE_STRING,
                                G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING);

    /* insert default */
    gtk_list_store_insert_with_values (store, &iter, position++,
                                       COLUMN_THEME_NAME, "default",
                                       COLUMN_THEME_DISPLAY_NAME, _("Default"), -1);

    /* store the default path, so we always select a theme */
    active_path = gtk_tree_model_get_path (GTK_TREE_MODEL (store), &iter);

    if (G_LIKELY (basedirs))
    {
        /* walk the base directories */
        for (i = 0; basedirs[i] != NULL; i++)
        {
            /* init */
            homedir = NULL;

            /* parse the homedir if needed */
            if (strstr (basedirs[i], "~/") != NULL)
                path = homedir = g_strconcat (g_get_home_dir (), basedirs[i] + 1, NULL);
            else
                path = basedirs[i];

            /* open directory */
            dir = g_dir_open (path, 0, NULL);
            if (G_LIKELY (dir))
            {
                for (;;)
                {
                    /* get the directory name */
                    theme = g_dir_read_name (dir);
                    if (G_UNLIKELY (theme == NULL))
                        break;

                    /* build the full cursor path */
                    filename = g_build_filename (path, theme, "cursors", NULL);

                    /* check if it looks like a cursor theme */
                    if (g_file_test (filename, G_FILE_TEST_IS_DIR))
                    {
                        /* try to load a pixbuf */
                        pixbuf = mouse_settings_themes_preview_icon (filename);

                        /* insert in the store */
                        gtk_list_store_insert_with_values (store, &iter, position++,
                                                           COLUMN_THEME_PIXBUF, pixbuf,
                                                           COLUMN_THEME_NAME, theme,
                                                           COLUMN_THEME_DISPLAY_NAME, theme,
                                                           COLUMN_THEME_PATH, filename, -1);

                        /* check if this is the active theme, set the path */
                        if (active_theme && strcmp (active_theme, theme) == 0)
                        {
                            gtk_tree_path_free (active_path);
                            active_path = gtk_tree_model_get_path (GTK_TREE_MODEL (store), &iter);
                        }

                        /* release pixbuf */
                        if (G_LIKELY (pixbuf))
                            g_object_unref (G_OBJECT (pixbuf));

                        /* check for a index.theme file for additional information */
                        index_file = g_build_filename (path, theme, "index.theme", NULL);
                        if (g_file_test (index_file, G_FILE_TEST_IS_REGULAR))
                        {
                            /* open theme desktop file */
                            rc = xfce_rc_simple_open (index_file, TRUE);
                            if (G_LIKELY (rc))
                            {
                                /* check for the theme group */
                                if (xfce_rc_has_group (rc, "Icon Theme"))
                                {
                                    /* set group */
                                    xfce_rc_set_group (rc, "Icon Theme");

                                    /* read values */
                                    name = xfce_rc_read_entry (rc, "Name", theme);
                                    comment = xfce_rc_read_entry (rc, "Comment", NULL);

                                    /* escape the comment */
                                    comment_escaped = comment ? g_markup_escape_text (comment, -1) : NULL;

                                    /* update store */
                                    gtk_list_store_set (store, &iter,
                                                        COLUMN_THEME_DISPLAY_NAME, name,
                                                        COLUMN_THEME_COMMENT, comment_escaped, -1);

                                    /* cleanup */
                                    g_free (comment_escaped);
                                }

                                /* close rc file */
                                xfce_rc_close (rc);
                            }
                        }

                        /* cleanup */
                        g_free (index_file);
                    }

                    /* cleanup */
                    g_free (filename);
                }

                /* close directory */
                g_dir_close (dir);
            }

            /* cleanup */
            g_free (homedir);
        }

        /* cleanup */
        g_strfreev (basedirs);
    }

    /* cleanup */
    g_free (active_theme);

    /* setup the columns */
    renderer = gtk_cell_renderer_pixbuf_new ();
    column = gtk_tree_view_column_new_with_attributes ("", renderer, "pixbuf", COLUMN_THEME_PIXBUF, NULL);

    renderer = gtk_cell_renderer_text_new ();
    g_object_set (G_OBJECT (renderer), "ellipsize", PANGO_ELLIPSIZE_END, NULL);

    /* sort the store */
    gtk_tree_sortable_set_sort_func (GTK_TREE_SORTABLE (store), COLUMN_THEME_DISPLAY_NAME, mouse_settings_themes_sort_func, NULL, NULL);
    gtk_tree_sortable_set_sort_column_id (GTK_TREE_SORTABLE (store), COLUMN_THEME_DISPLAY_NAME, GTK_SORT_ASCENDING);

    /* release the store */
    return store;
}
