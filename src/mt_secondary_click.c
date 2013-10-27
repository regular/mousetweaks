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

#include "mt_secondary_click.h"

typedef struct
{
    gint  x;
    gint  y;
    guint activate : 1;
} MtSecondaryClickPrivate;

struct _MtSecondaryClick
{
    MtClick                  parent;
    MtSecondaryClickPrivate *priv;
};

G_DEFINE_TYPE_WITH_PRIVATE (MtSecondaryClick, mt_secondary_click, MT_TYPE_CLICK)

static void
mt_secondary_click_init (MtSecondaryClick *click)
{
    click->priv = mt_secondary_click_get_instance_private (click);
}

static void
mt_secondary_click_timer_finished (MtClick    *click,
                                   MtTimer    *timer,
                                   MtListener *listener)
{
    MtSecondaryClickPrivate *priv = MT_SECONDARY_CLICK (click)->priv;

    priv->activate = TRUE;
}

static void
mt_secondary_click_motion_event (MtClick    *click,
                                 MtListener *listener,
                                 MtEvent    *event,
                                 MtTimer    *timer)
{
    MtSecondaryClickPrivate *priv = MT_SECONDARY_CLICK (click)->priv;
    gint t;

    if (!mt_timer_is_running (timer))
        return;

    g_object_get (click, "threshold", &t, NULL);

    if (ABS (priv->x - event->x) > t || ABS (priv->y - event->y) > t)
    {
        mt_timer_stop (timer);
        priv->activate = FALSE;
    }
}

static void
mt_secondary_click_button_event (MtClick    *click,
                                 MtListener *listener,
                                 MtEvent    *event,
                                 MtTimer    *timer)
{
    MtSecondaryClickPrivate *priv = MT_SECONDARY_CLICK (click)->priv;

    if (event->button != 1)
        return;

    if (event->type == MT_EVENT_BUTTON_PRESS)
    {
        priv->x = event->x;
        priv->y = event->y;
        mt_timer_start (timer);
    }
    else
    {
        mt_timer_stop (timer);
        if (priv->activate)
        {
            mt_listener_send_event (listener, 3, MT_SEND_CLICK);
            priv->activate = FALSE;
        }
    }
}

static void
mt_secondary_click_class_init (MtSecondaryClickClass *klass)
{
    MtClickClass *click_class = MT_CLICK_CLASS (klass);

    click_class->button_event = mt_secondary_click_button_event;
    click_class->motion_event = mt_secondary_click_motion_event;
    click_class->timer_finished = mt_secondary_click_timer_finished;
}

MtSecondaryClick *
mt_secondary_click_new (void)
{
    MtSecondaryClick *click;
    const gint default_threshold = 10;

    click = g_object_new (MT_TYPE_SECONDARY_CLICK, NULL);

    mt_click_bind_setting (MT_CLICK (click), "enabled", "secondary-click-enabled");
    mt_click_bind_setting (MT_CLICK (click), "time", "secondary-click-time");
    g_object_set (click, "threshold", default_threshold, NULL);

    return click;
}
