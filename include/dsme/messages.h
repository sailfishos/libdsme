/**
   @file messages.h

   Message type definitions for DSME.
   <p>
   Copyright (C) 2004-2009 Nokia Corporation.

   @author Ari Saastamoinen
   @author Ismo Laitinen <ismo.laitinen@nokia.com>
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

#ifndef DSME_MESSAGES_H
#define DSME_MESSAGES_H

#include <stddef.h>
#include <sys/types.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif


#define DSME_MSG_NEW(T) \
  (T*)dsmemsg_new(DSME_MSG_ID_(T), sizeof(T), 0)

#define DSME_MSG_NEW_WITH_EXTRA(T, E) \
  (T*)dsmemsg_new(DSME_MSG_ID_(T), sizeof(T), (E))

#ifdef __cplusplus
#define DSME_MSG_INIT(T)               \
  (T){                                 \
    /* line_size_ = */ sizeof(T),      \
    /* size_      = */ sizeof(T),      \
    /* type_      = */ DSME_MSG_ID_(T) \
  }
#else
#define DSME_MSG_INIT(T)         \
  (T){                           \
    .line_size_ = sizeof(T),      \
    .size_      = sizeof(T),      \
    .type_      = DSME_MSG_ID_(T) \
  }
#endif


#define DSME_MSG_ID_(T)      T ## _ID_
#define DSME_MSG_ENUM(T, ID) DSME_MSG_ID_(T) = ID


/* TODO: either rename DSMEMSG->DSME_MSG or DSME_MSG->DSMEMSG */
#define DSMEMSG_EXTRA(M)                                              \
  (((const dsmemsg_generic_t*)(M))->line_size_ -                      \
   ((const dsmemsg_generic_t*)(M))->size_ > 0 ?                       \
     (void*)(((char*)(M)) + ((const dsmemsg_generic_t*)(M))->size_) : \
     (void*)0)

#define DSMEMSG_EXTRA_SIZE(M)                       \
  (((const dsmemsg_generic_t*)(M))->line_size_ -    \
    ((const dsmemsg_generic_t*)(M))->size_ > 0 ?    \
      ((const dsmemsg_generic_t*)(M))->line_size_ - \
        ((const dsmemsg_generic_t*)(M))->size_ :    \
      0)

#define DSMEMSG_CAST(T, M)                                       \
 ((((const dsmemsg_generic_t*)(M))->size_ == sizeof(T)) ?        \
    (((const dsmemsg_generic_t*)(M))->type_ == DSME_MSG_ID_(T) ? \
      (T*)(M) : 0) : 0)


/**
 * @defgroup message_if Message passing interface for modules
 * In addition to these functions, dsmesock_send() can be used from modules.
 */

#define DSMEMSG_PRIVATE_FIELDS \
  uint32_t line_size_;        \
  uint32_t size_;             \
  uint32_t type_;

/**
   Generic message type
   @ingroup message_if
*/
typedef struct dsmemsg_generic_t {
  DSMEMSG_PRIVATE_FIELDS
} dsmemsg_generic_t;


/**
   Specific message type that is sent when socket is closed
   @ingroup message_if
*/
typedef struct {
  DSMEMSG_PRIVATE_FIELDS
  uint8_t reason;
} DSM_MSGTYPE_CLOSE;

/**
   Close reasons
   @ingroup dsmesock_client
*/
enum {
  TSMSG_CLOSE_REASON_OOS = 0, /* Protocol out of sync (local) */
  TSMSG_CLOSE_REASON_EOF = 1, /* EOF read from socket (local) */
  TSMSG_CLOSE_REASON_REQ = 2, /* Peer requests close */
  TSMSG_CLOSE_REASON_ERR = 3  /* Undefined error conditions or read after close */
};


typedef dsmemsg_generic_t DSM_MSGTYPE_GET_VERSION;
typedef dsmemsg_generic_t DSM_MSGTYPE_DSME_VERSION;

/* TA stands for Type Approval: */
typedef dsmemsg_generic_t DSM_MSGTYPE_SET_TA_TEST_MODE;


enum {
    /* NOTE: dsme message types are defined in:
     * - libdsme
     * - libiphb
     * - dsme
     *
     * When adding new message types
     * 1) uniqueness of the identifiers must be
     *    ensured accross all these source trees
     * 2) the dsmemsg_id_name() function in libdsme
     *    must be made aware of the new message type
     */

    /* DSME Protocol messages 000000xx */
    DSME_MSG_ENUM(DSM_MSGTYPE_CLOSE,            0x00000001),

    /* misc */
    DSME_MSG_ENUM(DSM_MSGTYPE_GET_VERSION,      0x00001100),
    DSME_MSG_ENUM(DSM_MSGTYPE_DSME_VERSION,     0x00001101),
    DSME_MSG_ENUM(DSM_MSGTYPE_SET_TA_TEST_MODE, 0x00001102),
};

/** Allocate new dsme message object
 *
 * @note It is expected that application code does not
 *       make calls to this function.
 *
 * @note Really, do not call this function.
 *
 * @sa #DSME_MSG_NEW_WITH_EXTRA()
 * @sa #DSME_MSG_NEW()
 * @sa #dsmeipc_send_full()
 * @sa #dsmeipc_send_with_string()
 * @sa #dsmesock_send_with_extra()
 *
 *
 * @param id     message type identifier
 * @param size   message body size
 * @param extra  space to reserve for extra data
 */
void *dsmemsg_new(uint32_t id, size_t size, size_t extra);

/** Get dsme message type identifier
 *
 * @param msg message pointer
 *
 * @return type identifier
 */
uint32_t dsmemsg_id(const dsmemsg_generic_t *msg);

/** Get human readable name of dsme message type
 *
 * @param msg message pointer, or NULL
 *
 * @return name of the message type, or "NULL_MESSAGE"
 *
 * @sa dsmemsg_id_name()
 */
const char *dsmemsg_name(const dsmemsg_generic_t *msg);

/** Get dsme message body size
 *
 * @param msg message pointer
 *
 * @return size of the message body
 */
size_t dsmemsg_size(const dsmemsg_generic_t *msg);

/** Get dsme message line size
 *
 * @param msg message pointer
 *
 * @return total size occupied by the message
 */
size_t dsmemsg_line_size(const dsmemsg_generic_t *msg);

/** Get dsme message extra size
 *
 * @param msg message pointer
 *
 * @return size of extra data attached to the message
 *
 * @sa dsmemsg_extra_data()
 */
size_t dsmemsg_extra_size(const dsmemsg_generic_t *msg);

/** Get pointer to dsme message extra data
 *
 * @param msg message pointer
 *
 * @return pointer to extra data attached to the message,
 *         or NULL
 *
 * @sa dsmemsg_extra_size()
 */
const void *dsmemsg_extra_data(const dsmemsg_generic_t *msg);

/** Get human readable name of dsme message type identifier
 *
 * @note This function is meant to be used only for purposes
 *       of diagnostic logging.
 *
 * For known message types returns true const string.
 *
 * For unknown types statically allocated buffer is
 * used to return "UNKNOWN_<ID-IN-HEX>" type string.
 *
 * @param id message type identifier
 *
 * @return human readable message type name
 *
 * @sa dsmemsg_name()
 */
const char * dsmemsg_id_name(uint32_t id);

#ifdef __cplusplus
}
#endif

#endif
