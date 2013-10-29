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

#include <string.h>
#include <atspi/atspi-event-listener.h>
#include <atspi/atspi-registry.h>
#include <atspi/atspi-misc.h>

#include "mt_listener_atspi.h"

typedef struct
{
    AtspiEventListener *motion;
    AtspiEventListener *button;
    gint                x;
    gint                y;
} MtListenerAtspiPrivate;

struct _MtListenerAtspi
{
    MtListener              parent;
    MtListenerAtspiPrivate *priv;
};

G_DEFINE_TYPE_WITH_PRIVATE (MtListenerAtspi, mt_listener_atspi, MT_TYPE_LISTENER)

static void
handle_motion_event (AtspiEvent      *event,
                     MtListenerAtspi *listener)
{
    listener->priv->x = event->detail1;
    listener->priv->y = event->detail2;

    mt_listener_emit_motion_event (MT_LISTENER (listener),
                                   listener->priv->x,
                                   listener->priv->y);
}

static void
handle_button_event (AtspiEvent      *event,
                     MtListenerAtspi *listener)
{
    if (strlen (event->type) != 15)
        return;

    listener->priv->x = event->detail1;
    listener->priv->y = event->detail2;

    mt_listener_emit_button_event (MT_LISTENER (listener),
                                   *(event->type + 14) == 'p' ? MT_EVENT_BUTTON_PRESS :
                                                                MT_EVENT_BUTTON_RELEASE,
                                   *(event->type + 13) - 0x30,
                                   listener->priv->x,
                                   listener->priv->y);
}

static void
mt_listener_atspi_init (MtListenerAtspi *listener)
{
    listener->priv = mt_listener_atspi_get_instance_private (listener);

    if (atspi_init () != 0)
    {
        g_warning ("Failed to initialize AT-SPI2.");
        return;
    }

    listener->priv->motion = atspi_event_listener_new
        ((AtspiEventListenerCB) handle_motion_event, listener, NULL);

    if (!atspi_event_listener_register (listener->priv->motion,
                                        "mouse:abs", NULL))
    {
        g_warning ("Failed to register mouse:abs events.");
        return;
    }

    listener->priv->button = atspi_event_listener_new
        ((AtspiEventListenerCB) handle_button_event, listener, NULL);

    if (!atspi_event_listener_register (listener->priv->button,
                                        "mouse:button", NULL))
    {
        g_warning ("Failed to register mouse:button events.");
        return;
    }
}

static void
mt_listener_atspi_dispose (GObject *object)
{
    MtListenerAtspiPrivate *priv = MT_LISTENER_ATSPI (object)->priv;

    if (priv->motion)
    {
        atspi_event_listener_deregister (priv->motion, "mouse:abs", NULL);
        g_clear_object (&priv->motion);
    }

    if (priv->button)
    {
        atspi_event_listener_deregister (priv->button, "mouse:button", NULL);
        g_clear_object (&priv->motion);
    }

    G_OBJECT_CLASS (mt_listener_atspi_parent_class)->dispose (object);
}

static void
mt_listener_atspi_finalize (GObject *object)
{
    gint leaks;

    if ((leaks = atspi_exit ()))
        g_warning ("AT-SPI reported %i leaks.", leaks);

    G_OBJECT_CLASS (mt_listener_atspi_parent_class)->finalize (object);
}

static void
mt_listener_atspi_query_pointer (MtListener *listener,
                                 gint       *x,
                                 gint       *y)
{
    MtListenerAtspiPrivate *priv = MT_LISTENER_ATSPI (listener)->priv;

    if (x)
        *x = priv->x;

    if (y)
        *y = priv->y;
}

static void
mt_listener_atspi_send_event (MtListener *listener,
                              gint        button,
                              MtSendType  type)
{
    MtListenerAtspiPrivate *priv = MT_LISTENER_ATSPI (listener)->priv;
    gchar name[4];

    g_return_if_fail (button >= 1 && button <= 3);

    name[0] = 'b';
    name[1] = (gchar) button + 0x30;
    name[3] = '\0';

    switch (type)
    {
        case MT_SEND_CLICK:
            name[2] = 'c';
            break;
        case MT_SEND_DOUBLE_CLICK:
            name[2] = 'd';
            break;
        case MT_SEND_BUTTON_PRESS:
            name[2] = 'p';
            break;
        case MT_SEND_BUTTON_RELEASE:
            name[2] = 'r';
            break;
        default:
            g_warning ("Unknown SendType.");
            return;
    }
    atspi_generate_mouse_event (priv->x, priv->y, name, NULL);
}

static void
mt_listener_atspi_class_init (MtListenerAtspiClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);
    MtListenerClass *listener_class = MT_LISTENER_CLASS (klass);

    object_class->dispose = mt_listener_atspi_dispose;
    object_class->finalize = mt_listener_atspi_finalize;

    listener_class->query_pointer = mt_listener_atspi_query_pointer;
    listener_class->send_event = mt_listener_atspi_send_event;
}

MtListener *
mt_listener_atspi_new (void)
{
    return g_object_new (MT_TYPE_LISTENER_ATSPI, NULL);
}
