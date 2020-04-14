/**
   @file message.c

   Implements DSME message methods.
   <p>
   Copyright (C) 2004-2009 Nokia Corporation.
   Copyright (C) 2015-2018 Jolla Ltd.

   @author Semi Malinen <semi.malinen@nokia.com>
   @author Simo Piiroinen <simo.piiroinen@jollamobile.com>

   This file is part of Dsme.

   Dsme is free software; you can redistribute it and/or modify
   it under the terms of the GNU Lesser General Public License
   version 2.1 as published by the Free Software Foundation.

   Dsme is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with Dsme.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "include/dsme/messages.h"

#include <stdio.h>
#include <stdlib.h>

/** Lookup table for message type id <-> name
 *
 * Due to the way dsme message types are defined
 * (i.e. scattered in multiple enums across several
 * separate source trees), this must be hardcoded.
 */
static const struct
{
    const char *name;
    uint32_t   id;
} dsmemsg_id_lut[] =
{
    { "CLOSE",                          0x00000001 },
    { "DBUS_CONNECT",                   0x00000100 },
    { "DBUS_DISCONNECT",                0x00000101 },
    { "DBUS_CONNECTED",                 0x00000102 },
    { "STATE_CHANGE_IND",               0x00000301 },
    { "STATE_QUERY",                    0x00000302 },
    { "SAVE_DATA_IND",                  0x00000304 },
    { "POWERUP_REQ",                    0x00000305 },
    { "SHUTDOWN_REQ",                   0x00000306 },
    { "SET_ALARM_STATE",                0x00000307 },
    { "REBOOT_REQ",                     0x00000308 },
    { "STATE_REQ_DENIED_IND",           0x00000309 },
    { "THERMAL_SHUTDOWN_IND",           0x00000310 },
    { "SET_CHARGER_STATE",              0x00000311 },
    { "SET_THERMAL_STATE",              0x00000312 },
    { "SET_EMERGENCY_CALL_STATE",       0x00000313 },
    { "SET_BATTERY_STATE",              0x00000314 },
    { "BATTERY_EMPTY_IND",              0x00000315 },
    { "SHUTDOWN",                       0x00000316 },
    { "SET_USB_STATE",                  0x00000317 },
    { "TELINIT",                        0x00000318 },
    { "CHANGE_RUNLEVEL",                0x00000319 },
    { "SET_BATTERY_LEVEL",              0x0000031a },
    { "SET_THERMAL_STATUS",             0x00000320 },
    { "PROCESSWD_CREATE",               0x00000500 },
    { "PROCESSWD_DELETE",               0x00000501 },
    { "PROCESSWD_CLEAR",                0x00000502 },
    { "PROCESSWD_SET_INTERVAL",         0x00000503 },
    { "PROCESSWD_PING",                 0x00000504 },
    { "PROCESSWD_MANUAL_PING",          0x00000505 },
    { "PROCESSWD_PONG",                 0x00000506 },
    { "WAIT",                           0x00000600 },
    { "WAKEUP",                         0x00000601 },
    { "HEARTBEAT",                      0x00000702 },
    { "ENTER_MALF",                     0x00000900 },
    { "GET_VERSION",                    0x00001100 },
    { "DSME_VERSION",                   0x00001101 },
    { "SET_TA_TEST_MODE",               0x00001102 },
    { "SET_LOGGING_VERBOSITY",          0x00001103 },
    { "ADD_LOGGING_INCLUDE",            0x00001104 },
    { "ADD_LOGGING_EXCLUDE",            0x00001105 },
    { "USE_LOGGING_DEFAULTS",           0x00001106 },
    { "IDLE",                           0x00001337 },
    { "DISK_SPACE",                     0x00002000 },
    // sentinel
    { 0,                                0x00000000 }
};

const char *
dsmemsg_id_name(uint32_t id)
{
    for( size_t i = 0; dsmemsg_id_lut[i].name; ++i ) {
        if( dsmemsg_id_lut[i].id == id )
            return dsmemsg_id_lut[i].name;
    }

    static char buf[32];
    snprintf(buf, sizeof buf, "UNKNOWN_%08lx",
             (unsigned long)id);
    return buf;
}

void *
dsmemsg_new(uint32_t id, size_t size, size_t extra)
{
    dsmemsg_generic_t *msg = calloc(1, size + extra);
    if (msg == NULL) {
        /* TODO */
        exit(EXIT_FAILURE);
    }

    msg->line_size_ = size + extra;
    msg->size_      = size;
    msg->type_      = id;

    return msg;
}

uint32_t dsmemsg_id(const dsmemsg_generic_t *msg)
{
  return msg->type_;
}

const char *
dsmemsg_name(const dsmemsg_generic_t *msg)
{
    if( !msg )
        return "NULL_MESSAGE";
    return dsmemsg_id_name(dsmemsg_id(msg));
}

size_t
dsmemsg_size(const dsmemsg_generic_t *msg)
{
    return msg->size_;
}

size_t
dsmemsg_line_size(const dsmemsg_generic_t *msg)
{
    return msg->line_size_;
}

size_t
dsmemsg_extra_size(const dsmemsg_generic_t *msg)
{
    size_t body = dsmemsg_size(msg);
    size_t line = dsmemsg_line_size(msg);
    return (line > body) ? (line - body) : 0;
}

const void *
dsmemsg_extra_data(const dsmemsg_generic_t *msg)
{
    const void *data = 0;
    size_t extra = dsmemsg_extra_size(msg);

    if( extra > 0 ) {
        data = (char *)msg + dsmemsg_size(msg);
    }

    return data;
}
