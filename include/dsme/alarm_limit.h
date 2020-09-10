/**
   @file alarm_limit.h

   Alarm limit definitions for DSME.
   <p>
   Copyright (C) 2011 Nokia Corporation.

   @author Semi Malinen <semi.malinen@nokia.com>

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

#ifndef DSME_ALARM_LIMIT_H
#define DSME_ALARM_LIMIT_H

#ifdef __cplusplus
extern "C" {
#endif

/**
   Function returns the maximum time to next alarm that will make
   dsme convert a shutdown to actdead.
   @return SNOOZE_TIMEOUT
*/
int dsme_snooze_timeout_in_seconds(void);

#ifdef __cplusplus
};
#endif

#endif
