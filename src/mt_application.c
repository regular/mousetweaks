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

#include "config.h"
#include "mt_application.h"
#include "mt_hover_click.h"
#include "mt_secondary_click.h"

typedef struct
{
    MtHoverClick     *hover_click;
    MtSecondaryClick *secondary_click;

    guint             service_id;
    guint             watch_id;

    /* command-line arguments */
    gboolean          sc_enabled;
    gdouble           sc_time;
    gint              sc_threshold;
    gboolean          hc_enabled;
    gdouble           hc_time;
    gint              hc_threshold;
    gboolean          shutdown;
} MtApplicationPrivate;

struct _MtApplication
{
    GApplication          parent;
    MtApplicationPrivate *priv;
};

static const gchar introspection[] =
    "<node>"
    "  <interface name='org.gnome.Mousetweaks'>"
    "    <property type='i' name='ClickType' access='readwrite'/>"
    "  </interface>"
    "</node>";


G_DEFINE_TYPE_WITH_PRIVATE (MtApplication, mt_application, G_TYPE_APPLICATION)

static void
mt_application_init (MtApplication *app)
{
    app->priv = mt_application_get_instance_private (app);
}

static void
mt_application_dispose (GObject *object)
{
    MtApplicationPrivate *priv = MT_APPLICATION (object)->priv;

    g_clear_object (&priv->hover_click);
    g_clear_object (&priv->secondary_click);

    G_OBJECT_CLASS (mt_application_parent_class)->dispose (object);
}

static GVariant *
handle_get_property (GDBusConnection *connection,
                     const gchar     *sender,
                     const gchar     *path,
                     const gchar     *interface,
                     const gchar     *property,
                     GError         **error,
                     MtApplication   *app)
{
    if (g_strcmp0 (property, "ClickType") == 0)
    {
        gint click_type;

        g_object_get (app->priv->hover_click, "click-type", &click_type, NULL);
        return g_variant_new_int32 (click_type);
    }
    return NULL;
}

static gboolean
handle_set_property (GDBusConnection *connection,
                     const gchar     *sender,
                     const gchar     *path,
                     const gchar     *interface,
                     const gchar     *property,
                     GVariant        *value,
                     GError         **error,
                     MtApplication   *app)
{
    if (g_strcmp0 (property, "ClickType") == 0)
        g_object_set (app->priv->hover_click,
                      "click-type",
                      g_variant_get_int32 (value), NULL);

    return TRUE;
}

static const GDBusInterfaceVTable interface_vtable =
{
    (GDBusInterfaceMethodCallFunc) NULL,
    (GDBusInterfaceGetPropertyFunc) handle_get_property,
    (GDBusInterfaceSetPropertyFunc) handle_set_property
};

static void
emit_property_changed (MtHoverClick *click,
                       GParamSpec   *pspec,
                       GApplication *application)
{
    GDBusConnection *connection;
    GVariantBuilder builder;
    GVariantBuilder inv_builder;
    GVariant *prop_v;
    const gchar *path;
    gint click_type;
    GError *error = NULL;

    g_variant_builder_init (&builder, G_VARIANT_TYPE_ARRAY);
    g_variant_builder_init (&inv_builder, G_VARIANT_TYPE ("as"));

    g_object_get (click, "click-type", &click_type, NULL);
    g_variant_builder_add (&builder, "{sv}", "ClickType",
                           g_variant_new_int32 (click_type));

    prop_v = g_variant_new ("(sa{sv}as)",
                            "org.gnome.Mousetweaks",
                            &builder, &inv_builder);

    connection = g_application_get_dbus_connection (application);
    path = g_application_get_dbus_object_path (application);

    if (!g_dbus_connection_emit_signal (connection, NULL, path,
                                        "org.freedesktop.DBus.Properties",
                                        "PropertiesChanged",
                                        prop_v, &error))
    {
        g_warning ("%s", error->message);
        g_error_free (error);
    }
}

static void
click_selection_appeared (GDBusConnection *connection,
                          const gchar     *name,
                          const gchar     *owner,
                          gpointer         data)
{
    MtApplication *app = data;
    g_object_set (app->priv->hover_click, "show-window", FALSE, NULL);
}

static void
click_selection_vanished (GDBusConnection *connection,
                          const gchar     *name,
                          gpointer         data)
{
    MtApplication *app = data;
    g_object_set (app->priv->hover_click, "show-window", TRUE, NULL);
}

static gboolean
mt_application_dbus_register (GApplication    *application,
                              GDBusConnection *connection,
                              const gchar     *object_path,
                              GError         **error)
{
    MtApplication *app = MT_APPLICATION (application);
    MtApplicationPrivate *priv = app->priv;
    GDBusNodeInfo *info;

    if (!G_APPLICATION_CLASS (mt_application_parent_class)->dbus_register (application,
                                                                           connection,
                                                                           object_path,
                                                                           error))
        return FALSE;

    if (!(info = g_dbus_node_info_new_for_xml (introspection, error)))
        return FALSE;

    priv->service_id =
        g_dbus_connection_register_object (connection,
                                           object_path,
                                           info->interfaces[0],
                                           &interface_vtable,
                                           app, NULL, error);

    g_dbus_node_info_unref (info);

    if (!priv->service_id)
        return FALSE;

    priv->watch_id =
        g_bus_watch_name_on_connection (connection,
                                        "org.gnome.Mousetweaks.ClickSelection",
                                        G_BUS_NAME_WATCHER_FLAGS_NONE,
                                        click_selection_appeared,
                                        click_selection_vanished,
                                        app, NULL);
    return TRUE;
}

static void
mt_application_dbus_unregister (GApplication    *application,
                                GDBusConnection *connection,
                                const gchar     *object_path)
{
    MtApplication *app = MT_APPLICATION (application);
    MtApplicationPrivate *priv = app->priv;

    if (priv->watch_id)
    {
        g_bus_unwatch_name (priv->watch_id);
        priv->watch_id = 0;
    }

    if (priv->service_id)
    {
        g_dbus_connection_unregister_object (connection, priv->service_id);
        priv->service_id = 0;
    }

    G_APPLICATION_CLASS (mt_application_parent_class)->dbus_unregister (application,
                                                                        connection,
                                                                        object_path);
}

static int
mt_application_command_line (GApplication            *application,
                             GApplicationCommandLine *cmdline)
{
    MtApplicationPrivate *priv = MT_APPLICATION (application)->priv;
    gchar **argv;
    gint i, argc;

    argv = g_application_command_line_get_arguments (cmdline, &argc);

    for (i = 0; i < argc; i++)
    {
        if (g_strcmp0 (argv[i], "-s") == 0)
        {
            g_application_release (application);
            g_application_command_line_print (cmdline, "%s\n", _("Shutdown successful."));

            if (priv->sc_enabled)
                g_object_set (priv->secondary_click, "enabled", FALSE, NULL);

            if (priv->hc_enabled)
                g_object_set (priv->hover_click, "enabled", FALSE, NULL);

            break;
        }
    }

    g_strfreev (argv);

    return 0;
}

static gboolean
mt_application_local_command_line (GApplication *application,
                                   gchar      ***arguments,
                                   gint         *exit_status)
{
    MtApplicationPrivate *priv = MT_APPLICATION (application)->priv;
    GError *error = NULL;
    GOptionContext *context;
    gchar **argv, **optv;
    gint argc, optc, i;
    gboolean version;
    GOptionEntry entries[] =
    {
        { "hover-click", 0, 0, G_OPTION_ARG_NONE, &priv->hc_enabled,
          N_("Enable Hover Click"), NULL },

        { "hover-click-time", 0, 0, G_OPTION_ARG_DOUBLE, &priv->hc_time,
          N_("Time to wait before a hover click"), "[0.5-3.0]" },

        { "hover-click-threshold", 0, 0, G_OPTION_ARG_INT, &priv->hc_threshold,
          N_("Ignore small pointer movements"), "[0-50]" },

        { "secondary-click", 0, 0, G_OPTION_ARG_NONE, &priv->sc_enabled,
          N_("Enable Secondary Click Emulation"), NULL },

        { "secondary-click-time", 0, 0, G_OPTION_ARG_DOUBLE, &priv->sc_time,
          N_("Time to wait before a secondary click"), "[0.5-3.0]" },

        { "secondary-click-threshold", 0, 0, G_OPTION_ARG_INT, &priv->sc_threshold,
          N_("Ignore small pointer movements"), "[0-50]" },

        { "version", 'v', 0, G_OPTION_ARG_NONE, &version,
          N_("Print version"), NULL },

        { "shutdown", 's', 0, G_OPTION_ARG_NONE, &priv->shutdown,
          N_("Shut down mousetweaks"), NULL },

        { NULL }
    };

    priv->hc_threshold = -1;
    priv->sc_threshold = -1;
    *exit_status = 0;

    argv = *arguments;
    argc = g_strv_length (argv);

    optv = g_new0 (gchar *, argc + 1);
    optc = argc;

    for (i = 0; i < argc; i++)
        optv[i] = argv[i];

    context = g_option_context_new (_("- GNOME Mouse accessibility service"));
    g_option_context_add_main_entries (context, entries, NULL);

    if (!g_option_context_parse (context, &optc, &optv, &error))
    {
        g_printerr ("%s\n", error->message);
        g_error_free (error);
        *exit_status = 1;
        goto out;
    }

    if (version)
    {
        g_print ("%s\n", VERSION);
        goto out;
    }

    if (!g_application_register (application, NULL, &error))
    {
        g_printerr ("%s\n", error->message);
        g_error_free (error);
        *exit_status = 1;
        goto out;
    }

    if (priv->shutdown)
    {
        if (g_application_get_is_remote (application))
        {
            /* forward to primary instance */
            for (i = 1; i < argc; i++)
            {
                g_free (argv[i]);
                argv[i] = NULL;
            }

            argv[1] = g_strdup ("-s");

            g_option_context_free (context);
            g_free (optv);

            return FALSE;
        }
        else
            g_print ("%s\n", _("Nothing to shut down."));
    }

    if (g_application_get_is_remote (application))
        g_print ("%s\n", _("Mousetweaks is already running."));

out:
    g_option_context_free (context);
    g_free (optv);

    return TRUE;
}

static void
mt_application_startup (GApplication *application)
{
    MtApplication *app = MT_APPLICATION (application);
    MtApplicationPrivate *priv = app->priv;

    G_APPLICATION_CLASS (mt_application_parent_class)->startup (application);

    if (priv->shutdown)
        return;

    g_application_hold (application);

    priv->hover_click = mt_hover_click_new ();
    priv->secondary_click = mt_secondary_click_new ();

    g_signal_connect (priv->hover_click, "notify::click-type",
                      G_CALLBACK (emit_property_changed), app);

    /* apply command-line options */
    if (priv->hc_enabled)
    {
        if (priv->hc_time >= 0.2 && priv->hc_time <= 3.0)
            g_object_set (priv->hover_click, "time", priv->hc_time, NULL);

        if (priv->hc_threshold >= 0 && priv->hc_threshold <= 30)
            g_object_set (priv->hover_click, "threshold", priv->hc_threshold, NULL);

        g_object_set (priv->hover_click, "enabled", TRUE, NULL);
    }

    if (priv->sc_enabled)
    {
        if (priv->sc_time >= 0.5 && priv->sc_time <= 3.0)
            g_object_set (priv->secondary_click, "time", priv->sc_time, NULL);

        if (priv->sc_threshold >= 0 && priv->sc_threshold <= 30)
            g_object_set (priv->secondary_click, "threshold", priv->sc_threshold, NULL);

        g_object_set (priv->secondary_click, "enabled", TRUE, NULL);
    }
}

static void
mt_application_class_init (MtApplicationClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);
    GApplicationClass *app_class = G_APPLICATION_CLASS (klass);

    object_class->dispose = mt_application_dispose;

    app_class->command_line = mt_application_command_line;
    app_class->local_command_line = mt_application_local_command_line;
    app_class->dbus_register = mt_application_dbus_register;
    app_class->dbus_unregister = mt_application_dbus_unregister;
    app_class->startup = mt_application_startup;
}

MtApplication *
mt_application_new (void)
{
    return g_object_new (MT_TYPE_APPLICATION,
                         "application-id", "org.gnome.Mousetweaks",
                         NULL);
}
