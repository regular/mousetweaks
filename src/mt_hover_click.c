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

#include <signal.h>

#include "config.h"
#include "mt_hover_click.h"

typedef struct
{
    gint  x;
    gint  y;
    gint  click_type;
    GPid  window_pid;
    guint show_window     : 1;
    guint window_launched : 1;
    guint drag_started    : 1;
} MtHoverClickPrivate;

struct _MtHoverClick
{
    MtClick              parent;
    MtHoverClickPrivate *priv;
};

enum
{
    PROP_0,
    PROP_CLICK_TYPE,
    PROP_SHOW_WINDOW
};

G_DEFINE_TYPE_WITH_PRIVATE (MtHoverClick, mt_hover_click, MT_TYPE_CLICK)

static void
mt_hover_click_init (MtHoverClick *click)
{
    click->priv = mt_hover_click_get_instance_private (click);
}

static void
mt_hover_click_timer_finished (MtClick    *click,
                               MtTimer    *timer,
                               MtListener *listener)
{
    MtHoverClickPrivate *priv = MT_HOVER_CLICK (click)->priv;

    if (priv->drag_started)
    {
        priv->drag_started = FALSE;
        mt_listener_send_event (listener, 1, MT_SEND_BUTTON_RELEASE);
        g_object_set (click, "click-type", MT_CLICK_TYPE_PRIMARY, NULL);
        return;
    }

    switch (priv->click_type)
    {
        case MT_CLICK_TYPE_PRIMARY:
            mt_listener_send_event (listener, 1, MT_SEND_CLICK);
            break;

        case MT_CLICK_TYPE_MIDDLE:
            mt_listener_send_event (listener, 2, MT_SEND_CLICK);
            g_object_set (click, "click-type", MT_CLICK_TYPE_PRIMARY, NULL);
            break;

        case MT_CLICK_TYPE_SECONDARY:
            mt_listener_send_event (listener, 3, MT_SEND_CLICK);
            g_object_set (click, "click-type", MT_CLICK_TYPE_PRIMARY, NULL);
            break;

        case MT_CLICK_TYPE_DOUBLE:
            mt_listener_send_event (listener, 1, MT_SEND_CLICK);
            mt_listener_send_event (listener, 1, MT_SEND_CLICK);
            g_object_set (click, "click-type", MT_CLICK_TYPE_PRIMARY, NULL);
            break;

        case MT_CLICK_TYPE_DRAG:
            mt_listener_send_event (listener, 1, MT_SEND_BUTTON_PRESS);
            priv->drag_started = TRUE;
            break;

        default:
            g_warning ("Unknown click-type selected.");
            break;
    }
}

static void
mt_hover_click_motion_event (MtClick    *click,
                             MtListener *listener,
                             MtEvent    *event,
                             MtTimer    *timer)
{
    MtHoverClickPrivate *priv = MT_HOVER_CLICK (click)->priv;
    gint t;

    g_object_get (click, "threshold", &t, NULL);

    if (priv->x == -1 && priv->y == -1)
    {
        priv->x = event->x;
        priv->y = event->y;
    }
    else if (ABS (priv->x - event->x) > t || ABS (priv->y - event->y) > t)
    {
        priv->x = event->x;
        priv->y = event->y;
        mt_timer_start (timer);
    }
}

static void
mt_hover_click_button_event (MtClick    *click,
                             MtListener *listener,
                             MtEvent    *event,
                             MtTimer    *timer)
{
    MtHoverClickPrivate *priv = MT_HOVER_CLICK (click)->priv;

    if (mt_timer_is_running (timer))
    {
        mt_timer_stop (timer);

        if (priv->drag_started)
            priv->drag_started = FALSE;

        if (priv->click_type != MT_CLICK_TYPE_PRIMARY)
            g_object_set (click, "click-type", MT_CLICK_TYPE_PRIMARY, NULL);
    }
}

static void
mt_hover_click_window_show (MtHoverClick *click)
{
    MtHoverClickPrivate *priv = click->priv;

    if (!priv->window_launched)
    {
        gchar *args[2] = { LIBEXECDIR "/hover-click-window", NULL };
        GError *error = NULL;

        g_spawn_async (NULL, args, NULL, 0, NULL, NULL,
                       &priv->window_pid, &error);
        if (error)
        {
            g_warning ("%s", error->message);
            g_error_free (error);
            return;
        }
        priv->window_launched = TRUE;
    }
}

static void
mt_hover_click_window_hide (MtHoverClick *click)
{
    MtHoverClickPrivate *priv = click->priv;

    if (priv->window_launched)
    {
        kill (priv->window_pid, SIGHUP);
        g_spawn_close_pid (priv->window_pid);
        priv->window_launched = FALSE;
    }
}

static void
mt_hover_click_set_property (GObject      *object,
                             guint         prop_id,
                             const GValue *value,
                             GParamSpec   *pspec)
{
    MtHoverClickPrivate *priv = MT_HOVER_CLICK (object)->priv;

    switch (prop_id)
    {
        case PROP_CLICK_TYPE:
            priv->click_type = g_value_get_int (value);
            g_object_notify_by_pspec (object, pspec);
            break;

        case PROP_SHOW_WINDOW:
            priv->show_window = g_value_get_boolean (value);
            g_object_notify_by_pspec (object, pspec);
            if (priv->show_window)
            {
                gboolean enabled;

                g_object_get (object, "enabled", &enabled, NULL);

                if (enabled)
                    mt_hover_click_window_show (MT_HOVER_CLICK (object));
            }
            else
                mt_hover_click_window_hide (MT_HOVER_CLICK (object));
            break;

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
mt_hover_click_get_property (GObject    *object,
                             guint       prop_id,
                             GValue     *value,
                             GParamSpec *pspec)
{
    MtHoverClickPrivate *priv = MT_HOVER_CLICK (object)->priv;

    switch (prop_id)
    {
        case PROP_CLICK_TYPE:
            g_value_set_int (value, priv->click_type);
            break;

        case PROP_SHOW_WINDOW:
            g_value_set_boolean (value, priv->show_window);
            break;

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
mt_hover_click_class_init (MtHoverClickClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);
    MtClickClass *click_class = MT_CLICK_CLASS (klass);

    object_class->get_property = mt_hover_click_get_property;
    object_class->set_property = mt_hover_click_set_property;

    click_class->button_event = mt_hover_click_button_event;
    click_class->motion_event = mt_hover_click_motion_event;
    click_class->timer_finished = mt_hover_click_timer_finished;

    g_object_class_install_property (object_class, PROP_CLICK_TYPE,
        g_param_spec_int ("click-type", "Click Type",
                          "The active click type",
                          MT_CLICK_TYPE_PRIMARY, MT_CLICK_TYPE_DRAG, MT_CLICK_TYPE_PRIMARY,
                          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

    g_object_class_install_property (object_class, PROP_SHOW_WINDOW,
        g_param_spec_boolean ("show-window", "Show Window",
                              "Whether to show the click selection window",
                              FALSE,
                              G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
}

static void
click_enabled_notify (MtClick    *click,
                      GParamSpec *pspec,
                      gpointer    data)
{
    MtHoverClickPrivate *priv = MT_HOVER_CLICK (click)->priv;
    gboolean enabled;

    g_object_get (click, "enabled", &enabled, NULL);

    if (enabled)
    {
        priv->x = -1;
        priv->y = -1;
        if (priv->show_window)
            mt_hover_click_window_show (MT_HOVER_CLICK (click));
    }
    else
        mt_hover_click_window_hide (MT_HOVER_CLICK (click));
}

MtHoverClick *
mt_hover_click_new (void)
{
    MtHoverClick *click;

    click = g_object_new (MT_TYPE_HOVER_CLICK, NULL);

    mt_click_bind_setting (MT_CLICK (click), "enabled", "dwell-click-enabled");
    mt_click_bind_setting (MT_CLICK (click), "threshold", "dwell-threshold");
    mt_click_bind_setting (MT_CLICK (click), "time", "dwell-time");

    g_signal_connect (click, "notify::enabled",
                      G_CALLBACK (click_enabled_notify), NULL);

    return click;
}
