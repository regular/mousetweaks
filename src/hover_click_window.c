/*
 * Copyright 2013, Gerd Kohlberger <gerdko gmail com>
 *
 * This file is part of Mousetweaks.
 *
 * Mousetweaks is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * Mousetweaks is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include <glib/gi18n.h>
#include <glib-unix.h>
#include <gtk/gtk.h>

#include "config.h"

#define HOVER_CLICK_TYPE_WINDOW (hover_click_window_get_type ())
#define HOVER_CLICK_WINDOW(o) (G_TYPE_CHECK_INSTANCE_CAST ((o), HOVER_CLICK_TYPE_WINDOW, HoverClickWindow))

typedef GtkApplicationWindowClass HoverClickWindowClass;

typedef struct
{
    GSettings        *settings;
    GDBusProxy       *proxy;
    GtkWidget        *box;

    GtkMenu          *menu;
    GtkMenuItem      *orientation;
    GtkCheckMenuItem *vertical;
    GtkCheckMenuItem *horizontal;

    GtkToggleButton  *button_primary;
    GtkToggleButton  *button_double;
    GtkToggleButton  *button_secondary;
    GtkToggleButton  *button_drag;
} HoverClickWindowPrivate;

typedef struct
{
    GtkApplicationWindow     parent;
    HoverClickWindowPrivate *priv;
} HoverClickWindow;

enum
{
    MT_CLICK_TYPE_PRIMARY,
    MT_CLICK_TYPE_MIDDLE,
    MT_CLICK_TYPE_SECONDARY,
    MT_CLICK_TYPE_DOUBLE,
    MT_CLICK_TYPE_DRAG
};

static GType hover_click_window_get_type (void) G_GNUC_CONST;

G_DEFINE_TYPE_WITH_PRIVATE (HoverClickWindow, hover_click_window, GTK_TYPE_APPLICATION_WINDOW)

static gboolean
menu_popup (GtkWidget        *widget,
            GdkEventButton   *ev,
            HoverClickWindow *win)
{
    if (ev->type == GDK_BUTTON_PRESS && ev->button == GDK_BUTTON_SECONDARY)
    {
        gtk_menu_popup_for_device (win->priv->menu,
                                   ev->device,
                                   NULL, NULL, NULL, NULL, NULL,
                                   ev->button,
                                   ev->time);
        return TRUE;
    }
    return FALSE;
}

static void
set_click_type (HoverClickWindow *win,
                gint              click_type)
{
    g_dbus_proxy_call (win->priv->proxy,
                       "org.freedesktop.DBus.Properties.Set",
                       g_variant_new ("(ssv)",
                                      "org.gnome.Mousetweaks",
                                      "ClickType",
                                      g_variant_new_int32 (click_type)),
                       G_DBUS_PROXY_FLAGS_NONE,
                       -1, NULL, NULL, NULL);
}

static void
button_primary_toggled (GtkToggleButton  *button,
                        HoverClickWindow *win)
{
    if (gtk_toggle_button_get_active (button))
        set_click_type (win, MT_CLICK_TYPE_PRIMARY);
}

static void
button_double_toggled (GtkToggleButton  *button,
                       HoverClickWindow *win)
{
    if (gtk_toggle_button_get_active (button))
        set_click_type (win, MT_CLICK_TYPE_DOUBLE);
}

static void
button_secondary_toggled (GtkToggleButton  *button,
                          HoverClickWindow *win)
{
    if (gtk_toggle_button_get_active (button))
        set_click_type (win, MT_CLICK_TYPE_SECONDARY);
}

static void
button_drag_toggled (GtkToggleButton  *button,
                     HoverClickWindow *win)
{
    if (gtk_toggle_button_get_active (button))
        set_click_type (win, MT_CLICK_TYPE_DRAG);
}

static void
click_type_changed (GDBusProxy       *proxy,
                    GVariant         *changed,
                    GStrv             invalidated,
                    HoverClickWindow *win)
{
    GtkToggleButton *b;
    GVariant *prop;

    if (!(prop = g_dbus_proxy_get_cached_property (proxy, "ClickType")))
        return;

    switch (g_variant_get_int32 (prop))
    {
        case MT_CLICK_TYPE_PRIMARY:
            b = win->priv->button_primary;

            g_signal_handlers_block_by_func (b, button_primary_toggled, win);
            gtk_toggle_button_set_active (b, TRUE);
            g_signal_handlers_unblock_by_func (b, button_primary_toggled, win);
            break;

        case MT_CLICK_TYPE_DOUBLE:
            b = win->priv->button_double;

            g_signal_handlers_block_by_func (b, button_double_toggled, win);
            gtk_toggle_button_set_active (b, TRUE);
            g_signal_handlers_unblock_by_func (b, button_double_toggled, win);
            break;

        case MT_CLICK_TYPE_SECONDARY:
            b = win->priv->button_secondary;

            g_signal_handlers_block_by_func (b, button_secondary_toggled, win);
            gtk_toggle_button_set_active (b, TRUE);
            g_signal_handlers_unblock_by_func (b, button_secondary_toggled, win);
            break;

        case MT_CLICK_TYPE_DRAG:
            b = win->priv->button_drag;

            g_signal_handlers_block_by_func (b, button_drag_toggled, win);
            gtk_toggle_button_set_active (b, TRUE);
            g_signal_handlers_unblock_by_func (b, button_drag_toggled, win);
            break;

        default:
            g_warning ("Unknown 'ClickType' value received.");
    }
    g_variant_unref (prop);
}

static void
mousetweaks_proxy_ready (GDBusConnection  *connection,
                         GAsyncResult     *result,
                         HoverClickWindow *win)
{
    GError *error = NULL;

    win->priv->proxy = g_dbus_proxy_new_finish (result, &error);

    if (error)
    {
        g_warning ("%s", error->message);
        g_error_free (error);
        gtk_widget_destroy (GTK_WIDGET (win));
        return;
    }

    g_signal_connect (win->priv->proxy, "g-properties-changed",
                      G_CALLBACK (click_type_changed), win);
}

static void
vertical_toggled (GtkCheckMenuItem *item,
                  HoverClickWindow *win)
{
    if (gtk_check_menu_item_get_active (item))
        gtk_orientable_set_orientation (GTK_ORIENTABLE (win->priv->box),
                                        GTK_ORIENTATION_VERTICAL);
}

static void
horizontal_toggled (GtkCheckMenuItem *item,
                    HoverClickWindow *win)
{
    if (gtk_check_menu_item_get_active (item))
        gtk_orientable_set_orientation (GTK_ORIENTABLE (win->priv->box),
                                        GTK_ORIENTATION_HORIZONTAL);
}

static void
orientation_notify (GObject          *object,
                    GParamSpec       *pspec,
                    HoverClickWindow *win)
{
    /* reset to minimum size */
    gtk_window_resize (GTK_WINDOW (win), 1, 1);
}

static gboolean
orientation_get_mapping (GValue   *value,
                         GVariant *variant,
                         gpointer  data)
{
    if (g_strcmp0 ("vertical", g_variant_get_string (variant, NULL)) == 0)
        g_value_set_enum (value, GTK_ORIENTATION_VERTICAL);
    else
        g_value_set_enum (value, GTK_ORIENTATION_HORIZONTAL);

    return TRUE;
}

static GVariant *
orientation_set_mapping (const GValue       *value,
                         const GVariantType *expected,
                         gpointer            data)
{
    if (g_value_get_enum (value) == GTK_ORIENTATION_VERTICAL)
        return g_variant_new_string ("vertical");
    else
        return g_variant_new_string ("horizontal");
}

static void
hover_click_window_dispose (GObject *object)
{
    HoverClickWindowPrivate *priv = HOVER_CLICK_WINDOW (object)->priv;

    g_clear_object (&priv->settings);
    g_clear_object (&priv->proxy);

    G_OBJECT_CLASS (hover_click_window_parent_class)->dispose (object);
}

static void
set_menu_header (HoverClickWindow *win)
{
    HoverClickWindowPrivate *priv = win->priv;
    PangoAttribute *attr;
    PangoAttrList *list;
    GtkWidget *child;

    child = gtk_bin_get_child (GTK_BIN (priv->orientation));
    list = pango_attr_list_new ();
    attr = pango_attr_weight_new (PANGO_WEIGHT_BOLD);
    pango_attr_list_insert (list, attr);
    gtk_label_set_attributes (GTK_LABEL (child), list);
    pango_attr_list_unref (list);
}

static void
apply_window_geometry (HoverClickWindow *win)
{
    gchar *geo;

    geo = g_settings_get_string (win->priv->settings, "window-geometry");
    gtk_window_parse_geometry (GTK_WINDOW (win), geo);
    g_free (geo);
}

static void
save_window_geometry (HoverClickWindow *win)
{
    gint x, y, w, h;
    gchar *geo;

    gtk_window_get_size (GTK_WINDOW (win), &w, &h);
    gtk_window_get_position (GTK_WINDOW (win), &x, &y);

    geo = g_strdup_printf ("%ix%i+%i+%i", w, h, x, y);
    g_settings_set_string (win->priv->settings, "window-geometry", geo);
    g_free (geo);
}

static void
hover_click_window_init (HoverClickWindow *win)
{
    HoverClickWindowPrivate *priv;

    win->priv = priv = hover_click_window_get_instance_private (win);
    win->priv->settings = g_settings_new ("org.gnome.Mousetweaks");

    gtk_widget_init_template (GTK_WIDGET (win));

    gtk_window_set_default_icon_name ("input-mouse");

    g_object_set (gtk_settings_get_default (),
                  "gtk-application-prefer-dark-theme", TRUE, NULL);

    g_settings_bind_with_mapping (priv->settings, "window-orientation",
                                  priv->box, "orientation",
                                  G_SETTINGS_BIND_DEFAULT,
                                  orientation_get_mapping,
                                  orientation_set_mapping,
                                  NULL, NULL);

    g_signal_connect (priv->box, "notify::orientation",
                      G_CALLBACK (orientation_notify), win);

    g_signal_connect (priv->button_primary, "toggled", G_CALLBACK (button_primary_toggled), win);
    g_signal_connect (priv->button_double, "toggled", G_CALLBACK (button_double_toggled), win);
    g_signal_connect (priv->button_secondary, "toggled", G_CALLBACK (button_secondary_toggled), win);
    g_signal_connect (priv->button_drag, "toggled", G_CALLBACK (button_drag_toggled), win);

    g_signal_connect (priv->button_primary, "button-press-event", G_CALLBACK (menu_popup), win);
    g_signal_connect (priv->button_double, "button-press-event", G_CALLBACK (menu_popup), win);
    g_signal_connect (priv->button_secondary, "button-press-event", G_CALLBACK (menu_popup), win);
    g_signal_connect (priv->button_drag, "button-press-event", G_CALLBACK (menu_popup), win);

    if (!g_settings_get_enum (priv->settings, "window-orientation"))
        gtk_check_menu_item_set_active (priv->vertical, TRUE);
    else
        gtk_check_menu_item_set_active (priv->horizontal, TRUE);

    g_signal_connect (priv->vertical, "toggled", G_CALLBACK (vertical_toggled), win);
    g_signal_connect (priv->horizontal, "toggled", G_CALLBACK (horizontal_toggled), win);

    set_menu_header (win);

    gtk_window_stick (GTK_WINDOW (win));
    gtk_window_set_keep_above (GTK_WINDOW (win), TRUE);
}

static void
hover_click_window_class_init (HoverClickWindowClass *klass)
{
    GtkWidgetClass *wc = GTK_WIDGET_CLASS (klass);

    G_OBJECT_CLASS (klass)->dispose = hover_click_window_dispose;

    gtk_widget_class_set_template_from_resource (wc, "/org/gnome/Mousetweaks/window.ui");
    gtk_widget_class_bind_template_child_private (wc, HoverClickWindow, box);
    gtk_widget_class_bind_template_child_private (wc, HoverClickWindow, menu);
    gtk_widget_class_bind_template_child_private (wc, HoverClickWindow, orientation);
    gtk_widget_class_bind_template_child_private (wc, HoverClickWindow, vertical);
    gtk_widget_class_bind_template_child_private (wc, HoverClickWindow, horizontal);
    gtk_widget_class_bind_template_child_private (wc, HoverClickWindow, button_primary);
    gtk_widget_class_bind_template_child_private (wc, HoverClickWindow, button_double);
    gtk_widget_class_bind_template_child_private (wc, HoverClickWindow, button_secondary);
    gtk_widget_class_bind_template_child_private (wc, HoverClickWindow, button_drag);
}

static void
sighup_received (HoverClickWindow *win)
{
    save_window_geometry (win);
    gtk_widget_destroy (GTK_WIDGET (win));
}

static void
application_activate (GApplication *app,
                      gpointer      data)
{
    GList *windows;
    GtkWidget *win;

    if ((windows = gtk_application_get_windows (GTK_APPLICATION (app))))
    {
        gtk_window_present (windows->data);
        return;
    }

    win = g_object_new (HOVER_CLICK_TYPE_WINDOW, NULL);
    gtk_application_add_window (GTK_APPLICATION (app), GTK_WINDOW (win));
    gtk_widget_show (win);

    apply_window_geometry (HOVER_CLICK_WINDOW (win));

    g_unix_signal_add (SIGHUP, (GSourceFunc) sighup_received, win);

    g_dbus_proxy_new (g_application_get_dbus_connection (app),
                      G_DBUS_PROXY_FLAGS_NONE,
                      NULL,
                      "org.gnome.Mousetweaks",
                      "/org/gnome/Mousetweaks",
                      "org.gnome.Mousetweaks",
                      NULL,
                      (GAsyncReadyCallback) mousetweaks_proxy_ready,
                      win);
}

int
main (int argc, char **argv)
{
    GtkApplication *app;
    int status;

    bindtextdomain (GETTEXT_PACKAGE, GNOMELOCALEDIR);
    bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
    textdomain (GETTEXT_PACKAGE);

    g_set_application_name (_("Hover Click"));

    app = gtk_application_new ("org.gnome.Mousetweaks.HoverClick", 0);
    g_signal_connect (app, "activate", G_CALLBACK (application_activate), NULL);
    status = g_application_run (G_APPLICATION (app), argc, argv);
    g_object_unref (app);

    return status;
}
