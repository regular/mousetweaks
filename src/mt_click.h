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

#ifndef __MT_CLICK_H__
#define __MT_CLICK_H__

#include <glib-object.h>

#include "mt_listener.h"
#include "mt_timer.h"

G_BEGIN_DECLS

#define MT_TYPE_CLICK         (mt_click_get_type ())
#define MT_CLICK(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), MT_TYPE_CLICK, MtClick))
#define MT_CLICK_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST ((k), MT_TYPE_CLICK, MtClickClass))
#define MT_IS_CLICK(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), MT_TYPE_CLICK))
#define MT_IS_CLICK_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), MT_TYPE_CLICK))
#define MT_CLICK_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), MT_TYPE_CLICK, MtClickClass))

typedef struct _MtClickPrivate MtClickPrivate;
typedef struct _MtClickClass   MtClickClass;
typedef struct _MtClick        MtClick;

struct _MtClickClass
{
    GObjectClass parent;

    void (* button_event)   (MtClick    *click,
                             MtListener *listener,
                             MtEvent    *event,
                             MtTimer    *timer);

    void (* motion_event)   (MtClick    *click,
                             MtListener *listener,
                             MtEvent    *event,
                             MtTimer    *timer);

    void (* timer_finished) (MtClick    *click,
                             MtTimer    *timer,
                             MtListener *listener);
};

struct _MtClick
{
    GObject         parent;
    MtClickPrivate *priv;
};

GType           mt_click_get_type       (void) G_GNUC_CONST;

void            mt_click_bind_setting   (MtClick     *click,
                                         const gchar *prop,
                                         const gchar *key);

MtListener *    mt_click_get_listener   (MtClick     *click);

MtTimer *       mt_click_get_timer      (MtClick     *click);

G_END_DECLS

#endif /* __MT_CLICK_H__ */
