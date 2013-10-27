/*
 * Copyright, 2013 Gerd Kohlberger <gerdko gmail com>
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

#include "mt_listener.h"
#include "mt_listener_atspi.h"

enum
{
    MOTION_EVENT,
    BUTTON_EVENT,
    LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = { 0, };

G_DEFINE_ABSTRACT_TYPE (MtListener, mt_listener, G_TYPE_OBJECT)

static void
mt_listener_init (MtListener *listener)
{
}

static void
mt_listener_class_init (MtListenerClass *klass)
{
    signals[MOTION_EVENT] =
        g_signal_new ("motion-event",
                      G_OBJECT_CLASS_TYPE (klass),
                      G_SIGNAL_RUN_LAST,
                      0, NULL, NULL,
                      g_cclosure_marshal_VOID__BOXED,
                      G_TYPE_NONE,
                      1, MT_TYPE_EVENT | G_SIGNAL_TYPE_STATIC_SCOPE);

    signals[BUTTON_EVENT] =
        g_signal_new ("button-event",
                      G_OBJECT_CLASS_TYPE (klass),
                      G_SIGNAL_RUN_LAST,
                      0, NULL, NULL,
                      g_cclosure_marshal_VOID__BOXED,
                      G_TYPE_NONE,
                      1, MT_TYPE_EVENT | G_SIGNAL_TYPE_STATIC_SCOPE);
}

static MtEvent *
mt_event_copy (const MtEvent *event)
{
    return g_memdup (event, sizeof (MtEvent));
}

static void
mt_event_free (MtEvent *event)
{
    g_free (event);
}

G_DEFINE_BOXED_TYPE (MtEvent, mt_event, mt_event_copy, mt_event_free)

MtListener *
mt_listener_get_default (void)
{
    static MtListener *listener = NULL;

    if (listener == NULL)
    {
        listener = mt_listener_atspi_new ();
        g_object_add_weak_pointer (G_OBJECT (listener), (gpointer *) &listener);
        return listener;
    }
    return g_object_ref (listener);
}

void
mt_listener_emit_button_event (MtListener *listener,
                               MtEventType type,
                               gint        button,
                               gint        x,
                               gint        y)
{
    MtEvent ev;

    ev.type = type;
    ev.button = button;
    ev.x = x;
    ev.y = y;

    g_signal_emit (listener, signals[BUTTON_EVENT], 0, &ev);
}

void
mt_listener_emit_motion_event (MtListener *listener,
                               gint        x,
                               gint        y)
{
    MtEvent ev;

    ev.type = MT_EVENT_MOTION;
    ev.button = 0;
    ev.x = x;
    ev.y = y;

    g_signal_emit (listener, signals[MOTION_EVENT], 0, &ev);
}

void
mt_listener_query_pointer (MtListener *listener,
                           gint       *x,
                           gint       *y)
{
    MtListenerClass *klass = MT_LISTENER_GET_CLASS (listener);

    if (klass->query_pointer)
        klass->query_pointer (listener, x, y);
}

void
mt_listener_send_event (MtListener *listener,
                        gint        button,
                        MtSendType  type)
{
    MtListenerClass *klass = MT_LISTENER_GET_CLASS (listener);

    if (klass->send_event)
        klass->send_event (listener, button, type);
}
