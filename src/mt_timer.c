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

#include "mt_timer.h"

typedef struct
{
    guint   id;
    gint64  stamp;
    gdouble target;
} MtTimerPrivate;

struct _MtTimer
{
    GObject         parent;
    MtTimerPrivate *priv;
};

enum
{
    FINISHED,
    LAST_SIGNAL
};

enum
{
    PROP_0,
    PROP_TARGET
};

static guint signals[LAST_SIGNAL] = { 0, };

G_DEFINE_TYPE_WITH_PRIVATE (MtTimer, mt_timer, G_TYPE_OBJECT)

static void
mt_timer_init (MtTimer *timer)
{
    timer->priv = mt_timer_get_instance_private (timer);
}

static void
mt_timer_get_property (GObject    *object,
                       guint       prop_id,
                       GValue     *value,
                       GParamSpec *pspec)
{
    MtTimer *timer = MT_TIMER (object);

    switch (prop_id)
    {
        case PROP_TARGET:
            g_value_set_double (value, timer->priv->target);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
mt_timer_set_property (GObject      *object,
                       guint         prop_id,
                       const GValue *value,
                       GParamSpec   *pspec)
{
    MtTimer *timer = MT_TIMER (object);

    switch (prop_id)
    {
        case PROP_TARGET:
            timer->priv->target = g_value_get_double (value);
            g_object_notify (object, pspec->name);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
mt_timer_finalize (GObject *object)
{
    mt_timer_stop (MT_TIMER (object));

    G_OBJECT_CLASS (mt_timer_parent_class)->finalize (object);
}

static void
mt_timer_class_init (MtTimerClass *klass)
{
    GObjectClass *object_class;

    object_class = G_OBJECT_CLASS (klass);
    object_class->get_property = mt_timer_get_property;
    object_class->set_property = mt_timer_set_property;
    object_class->finalize = mt_timer_finalize;

    signals[FINISHED] =
        g_signal_new ("finished",
                      G_OBJECT_CLASS_TYPE (klass),
                      G_SIGNAL_RUN_LAST,
                      0, NULL, NULL,
                      g_cclosure_marshal_VOID__VOID,
                      G_TYPE_NONE, 0);

    g_object_class_install_property (object_class, PROP_TARGET,
            g_param_spec_double ("target", "Target time",
                                 "Target time of the timer",
                                 0.1, 3.0, 0.1,
                                 G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
}

static gboolean
mt_timer_check (MtTimer *timer)
{
    gdouble elapsed;

    elapsed = (gdouble) (g_get_monotonic_time () - timer->priv->stamp) / 1e6;

    if (elapsed > timer->priv->target)
    {
        timer->priv->id = 0;
        g_signal_emit (timer, signals[FINISHED], 0);
        return G_SOURCE_REMOVE;
    }
    return G_SOURCE_CONTINUE;
}

MtTimer *
mt_timer_new (void)
{
    return g_object_new (MT_TYPE_TIMER, NULL);
}

void
mt_timer_start (MtTimer *timer)
{
    g_return_if_fail (MT_IS_TIMER (timer));

    if (timer->priv->id == 0)
        timer->priv->id = g_timeout_add (100, (GSourceFunc) mt_timer_check, timer);

    timer->priv->stamp = g_get_monotonic_time ();
}

void
mt_timer_stop (MtTimer *timer)
{
    g_return_if_fail (MT_IS_TIMER (timer));

    if (timer->priv->id)
    {
        g_source_remove (timer->priv->id);
        timer->priv->id = 0;
    }
}

gboolean
mt_timer_is_running (MtTimer *timer)
{
    g_return_val_if_fail (MT_IS_TIMER (timer), FALSE);

    return timer->priv->id != 0;
}
