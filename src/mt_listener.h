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

#ifndef __MT_LISTENER_H__
#define __MT_LISTENER_H__

#include <glib-object.h>

G_BEGIN_DECLS

#define MT_TYPE_EVENT            (mt_event_get_type ())
#define MT_TYPE_LISTENER         (mt_listener_get_type ())
#define MT_LISTENER(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), MT_TYPE_LISTENER, MtListener))
#define MT_LISTENER_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST ((k), MT_TYPE_LISTENER, MtListenerClass))
#define MT_IS_LISTENER(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), MT_TYPE_LISTENER))
#define MT_IS_LISTENER_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), MT_TYPE_LISTENER))
#define MT_LISTENER_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), MT_TYPE_LISTENER, MtListenerClass))

typedef GObject MtListener;

typedef enum
{
    MT_SEND_CLICK,
    MT_SEND_DOUBLE_CLICK,
    MT_SEND_BUTTON_PRESS,
    MT_SEND_BUTTON_RELEASE
} MtSendType;

typedef struct
{
    GObjectClass parent;

    void (* query_pointer) (MtListener *listener,
                            gint       *x,
                            gint       *y);

    void (* send_event)    (MtListener *listener,
                            gint        button,
                            MtSendType  type);
} MtListenerClass;

typedef enum
{
    MT_EVENT_MOTION,
    MT_EVENT_BUTTON_PRESS,
    MT_EVENT_BUTTON_RELEASE
} MtEventType;

typedef struct
{
    MtEventType type;
    gint        x;
    gint        y;
    gint        button;
} MtEvent;

GType           mt_event_get_type               (void) G_GNUC_CONST;
GType           mt_listener_get_type            (void) G_GNUC_CONST;
MtListener *    mt_listener_get_default         (void);

void            mt_listener_emit_button_event   (MtListener *listener,
                                                 MtEventType type,
                                                 gint        button,
                                                 gint        x,
                                                 gint        y);

void            mt_listener_emit_motion_event   (MtListener *listener,
                                                 gint        x,
                                                 gint        y);

void            mt_listener_query_pointer       (MtListener *listener,
                                                 gint       *x,
                                                 gint       *y);

void            mt_listener_send_event          (MtListener *listener,
                                                 gint        button,
                                                 MtSendType  type);

G_END_DECLS

#endif /* __MT_LISTENER_H__ */
