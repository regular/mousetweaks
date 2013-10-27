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

#include <gio/gio.h>

#include "mt_click.h"

struct _MtClickPrivate
{
    GSettings  *settings;
    MtListener *listener;
    MtTimer    *timer;

    gint        threshold;
    gdouble     time;
    guint       enabled : 1;
};

enum
{
    PROP_0,
    PROP_ENABLED,
    PROP_TIME,
    PROP_THRESHOLD
};

G_DEFINE_ABSTRACT_TYPE_WITH_PRIVATE (MtClick, mt_click, G_TYPE_OBJECT)

static void
timer_finished (MtTimer *timer,
                MtClick *click)
{
    MtClickClass *klass = MT_CLICK_GET_CLASS (click);

    if (click->priv->enabled && klass->timer_finished)
        klass->timer_finished (click, timer, click->priv->listener);
}

static void
listener_motion_event (MtListener *listener,
                       MtEvent    *event,
                       MtClick    *click)
{
    MtClickClass *klass = MT_CLICK_GET_CLASS (click);

    if (click->priv->enabled && klass->motion_event)
        klass->motion_event (click, listener, event, click->priv->timer);
}

static void
listener_button_event (MtListener *listener,
                       MtEvent    *event,
                       MtClick    *click)
{
    MtClickClass *klass = MT_CLICK_GET_CLASS (click);

    if (click->priv->enabled && klass->button_event)
        klass->button_event (click, listener, event, click->priv->timer);
}

static void
mt_click_init (MtClick *click)
{
    click->priv = mt_click_get_instance_private (click);
    click->priv->settings = g_settings_new ("org.gnome.desktop.a11y.mouse");
    click->priv->listener = mt_listener_get_default ();
    click->priv->timer = mt_timer_new ();

    g_signal_connect (click->priv->listener, "motion-event",
                      G_CALLBACK (listener_motion_event), click);
    g_signal_connect (click->priv->listener, "button-event",
                      G_CALLBACK (listener_button_event), click);
    g_signal_connect (click->priv->timer, "finished",
                      G_CALLBACK (timer_finished), click);

    g_object_bind_property (click, "time", click->priv->timer, "target", 0);
}

static void
mt_click_dispose (GObject *object)
{
    MtClickPrivate *priv = MT_CLICK (object)->priv;

    g_clear_object (&priv->settings);
    g_clear_object (&priv->listener);
    g_clear_object (&priv->timer);

    G_OBJECT_CLASS (mt_click_parent_class)->dispose (object);
}

static void
mt_click_set_property (GObject      *object,
                       guint         prop_id,
                       const GValue *value,
                       GParamSpec   *pspec)
{
    MtClickPrivate *priv = MT_CLICK (object)->priv;

    switch (prop_id)
    {
        case PROP_ENABLED:
            priv->enabled = g_value_get_boolean (value);
            g_object_notify_by_pspec (object, pspec);
            break;

        case PROP_TIME:
            priv->time = g_value_get_double (value);
            g_object_notify_by_pspec (object, pspec);
            break;

        case PROP_THRESHOLD:
            priv->threshold = g_value_get_int (value);
            g_object_notify_by_pspec (object, pspec);
            break;

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
mt_click_get_property (GObject    *object,
                       guint       prop_id,
                       GValue     *value,
                       GParamSpec *pspec)
{
    MtClickPrivate *priv = MT_CLICK (object)->priv;

    switch (prop_id)
    {
        case PROP_ENABLED:
            g_value_set_boolean (value, priv->enabled);
            break;

        case PROP_TIME:
            g_value_set_double (value, (gdouble) priv->time);
            break;

        case PROP_THRESHOLD:
            g_value_set_int (value, priv->threshold);
            break;

        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
mt_click_class_init (MtClickClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);

    object_class->dispose = mt_click_dispose;
    object_class->get_property = mt_click_get_property;
    object_class->set_property = mt_click_set_property;

    g_object_class_install_property (object_class, PROP_ENABLED,
        g_param_spec_boolean ("enabled", "Enabled",
                              "Enable click feature",
                              FALSE,
                              G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

    g_object_class_install_property (object_class, PROP_TIME,
        g_param_spec_double ("time", "Time",
                             "Acceptance delay for a click",
                             0.2, 3.0, 1.2,
                             G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

    g_object_class_install_property (object_class, PROP_THRESHOLD,
        g_param_spec_int ("threshold", "Motion threshold",
                          "Ignore pointer movement below this threshold",
                          0, 30, 10,
                          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
}

void
mt_click_bind_setting (MtClick     *click,
                       const gchar *prop,
                       const gchar *key)
{
    g_return_if_fail (MT_IS_CLICK (click));

    g_settings_bind (click->priv->settings, key, click, prop,
                     G_SETTINGS_BIND_DEFAULT | G_SETTINGS_BIND_NO_SENSITIVITY);
}

MtListener *
mt_click_get_listener (MtClick *click)
{
    g_return_val_if_fail (MT_IS_CLICK (click), NULL);
    return click->priv->listener;
}

MtTimer *
mt_click_get_timer (MtClick *click)
{
    g_return_val_if_fail (MT_IS_CLICK (click), NULL);
    return click->priv->timer;
}
