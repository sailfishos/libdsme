/**
   @file ut_libdsme.c

   Unit tests for libdsme.
   <p>
   Copyright (c) 2013 - 2022 Jolla Ltd.

   @author Martin Kampas <martin.kampas@tieto.com>
   @author Simo Piiroinen <simo.piiroinen@jolla.com>

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

/*
 * some glibc versions seems to mistakenly define ucred behind __USE_GNU;
 * work around by #defining _GNU_SOURCE
 */
#define _GNU_SOURCE

#include "../include/dsme/messages.h"
#include "../include/dsme/protocol.h"
#include "../include/dsme/state.h"

#include <sys/stat.h>
#include <sys/wait.h>

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>
#include <errno.h>
#include <poll.h>
#include <syslog.h>
#include <ctype.h>
#include <getopt.h>

#include <check.h>

/* ------------------------------------------------------------------------- *
 * Diagnostic Logging
 * ------------------------------------------------------------------------- */

static int log_level = LOG_WARNING;

static const char * const log_tag[] = {
    [LOG_EMERG]   = "X: ",
    [LOG_ALERT]   = "A: ",
    [LOG_CRIT]    = "C: ",
    [LOG_ERR]     = "E: ",
    [LOG_WARNING] = "W: ",
    [LOG_NOTICE]  = "N: ",
    [LOG_INFO]    = "I: ",
    [LOG_DEBUG]   = "D: ",
};

static void log_emit_(const char *file, int line, const char *func,
                      int level, const char *fmt, ...)
{
    int saved = errno;
    char *msg = NULL;
    va_list va;
    va_start(va, fmt);
    if( vasprintf(&msg, fmt, va) < 0 )
        msg = NULL;
    va_end(va);
    fprintf(stderr, "%s:%d: %s: %s%s\n",
            file, line, func,
            log_tag[level], msg ?: fmt);
    fflush(stderr);
    free(msg);
    errno = saved;
}

#define log_p(LEV) ((LEV)<=log_level)

#define log_emit(LEV, FMT,ARGS...)\
     do {\
         if( log_p(LEV) )\
             log_emit_(__FILE__, __LINE__, __func__, LEV, FMT, ##ARGS);\
     } while( false )

#define log_error(  FMT, ARGS...) log_emit(LOG_ERR,     FMT, ##ARGS)
#define log_warning(FMT, ARGS...) log_emit(LOG_WARNING, FMT, ##ARGS)
#define log_notice( FMT, ARGS...) log_emit(LOG_NOTICE,  FMT, ##ARGS)
#define log_info(   FMT, ARGS...) log_emit(LOG_INFO,    FMT, ##ARGS)
#define log_debug(  FMT, ARGS...) log_emit(LOG_DEBUG,   FMT, ##ARGS)

/* ------------------------------------------------------------------------- *
 * Utilities
 * ------------------------------------------------------------------------- */

static int wait_input(int fd)
{
    struct pollfd pfd = {
        .fd = fd,
        .events = POLLIN,
    };
    int rc = poll(&pfd, 1, 5000);
    if( rc == 0 )
        log_warning("wait_input() timeout");
    else if( rc == -1 )
        log_warning("wait_input() failed: %m");
    return rc;
}

/* ------------------------------------------------------------------------- *
 * Mock Daemon
 * ------------------------------------------------------------------------- */

static const char mock_socket[] = "/tmp/ut_libdsme.sock";
static const char mock_extra[] = "a-reason";

static int daemon_fd = -1;
static int daemon_pid = -1;

static bool daemon_handle_message(dsmesock_connection_t *connection)
{
    bool stay_connected = true;
    dsmemsg_generic_t *msg = dsmesock_receive(connection);

    log_notice("MOCK: recv(%s)", dsmemsg_name(msg));

    DSM_MSGTYPE_CLOSE *close_msg;
    DSM_MSGTYPE_STATE_QUERY *state_query_msg;

    if( !msg ) {
        /* Allocation failure / similar
         * Real daemon might retry later */
        log_error("MOCK: null message received");
        stay_connected = false;
    }
    else if( (close_msg = DSMEMSG_CAST(DSM_MSGTYPE_CLOSE, msg)) ) {
        /* Client disconnected */
        stay_connected = false;
    }
    else if( (state_query_msg = DSMEMSG_CAST(DSM_MSGTYPE_STATE_QUERY, msg)) ) {
        /* Dummy query from test_send_receive() */
        DSM_MSGTYPE_STATE_REQ_DENIED_IND reply =
            DSME_MSG_INIT(DSM_MSGTYPE_STATE_REQ_DENIED_IND);
        reply.state = DSME_STATE_TEST;
        log_notice("MOCK: send(%s)",
                   dsmemsg_name((dsmemsg_generic_t *)&reply));
        dsmesock_send_with_extra(connection, &reply,
                                 sizeof mock_extra, mock_extra);
    }

    free(msg);

    return stay_connected;
}

static bool daemon_handle_client(void)
{
    bool client_handled = false;
    int client_fd = -1;
    dsmesock_connection_t *connection = NULL;

    if( (client_fd = accept(daemon_fd, NULL, 0)) == -1 ) {
        log_error("MOCK: accept() failed: %m");
        goto bailout;
    }

    if( !(connection = dsmesock_init(client_fd)) ) {
        log_error("MOCK: dsmesock_init() failed");
        goto bailout;
    }
    /* Ownership of client_fd has been transferred to connection */
    client_fd = -1;

    bool stay_connected = true;
    while( stay_connected ) {
        if( wait_input(connection->fd) != 1 ) {
            log_error("MOCK: no data from client");
            goto bailout;
        }

        stay_connected = daemon_handle_message(connection);
    }

    client_handled = true;

bailout:
    if( connection )
        dsmesock_close(connection);

    if( client_fd != -1 )
        close(client_fd);

    log_debug("MOCK: client handled = %d", client_handled);

    return client_handled;
}

static void daemon_main(void)
{
    log_debug("MOCK: daemon running");
    for( ;; ) {
        log_info("MOCK: waiting client...");
        if( wait_input(daemon_fd) != 1 )
            break;
        log_info("MOCK: handling client...");
        if( !daemon_handle_client() )
            break;
    }
    log_error("MOCK: daemon stopped");
}

static bool daemon_start(void)
{
    bool success = false;

    if( unlink(mock_socket) == -1 && errno != ENOENT ) {
        log_error("MOCK: unlink(%s) failed: %m", mock_socket);
        goto bailout;
    }

    if( (daemon_fd = socket(PF_UNIX, SOCK_STREAM, 0)) == -1 ) {
        log_error("MOCK: socket() failed: %m");
        goto bailout;
    }

    struct sockaddr_un sa = {
        .sun_family = AF_UNIX,
    };
    strncat(sa.sun_path, mock_socket, sizeof sa.sun_path - 1);
    if( bind(daemon_fd, (struct sockaddr *)&sa, sizeof sa) == -1 ) {
        log_error("MOCK: bind(%s) failed: %m", mock_socket);
        goto bailout;
    }

    if( chmod(mock_socket, 0666) == -1 ) {
        log_error("MOCK: chmod(%s) failed: %m", mock_socket);
        goto bailout;
    }

    if( listen(daemon_fd, 5) == -1 ) {
        log_error("MOCK: listen(%s) failed: %m", mock_socket);
        goto bailout;
    }

    if( (daemon_pid = fork()) == -1 ) {
        log_error("MOCK: fork() failed: %m");
        goto bailout;
    }

    if( daemon_pid == 0 ) {
        /* Child proces = mock daemon */
        daemon_main();
        /* Expected: daemon process gets killed with SIGTERM
         *           and control does not return here. */
        _exit(EXIT_FAILURE);
    }

    success = true;

bailout:
    return success;
}

static void daemon_stop(void)
{
    if( unlink(mock_socket) == -1 && errno != ENOENT )
        log_warning("MOCK: unlink(%s) failed: %m", mock_socket);

    if( daemon_pid != -1 ) {
        if( kill(daemon_pid, SIGTERM) == -1 )
            log_warning("MOCK: daemon terminate failed: %m");

        int status = 0;
        int options = 0;
        if( waitpid(daemon_pid, &status, options) == -1 )
            log_warning("MOCK: daemon wait failed: %m");
        else if( WIFEXITED(status) )
            log_warning("MOCK: daemon terminated by exit(%d)",
                        WEXITSTATUS(status));
        else if( WIFSIGNALED(status) )
            log_debug("MOCK: daemon terminated by signal(%s)",
                      strsignal(WTERMSIG(status)));
        else
            log_warning("MOCK: daemon not terminated?");
        daemon_pid = -1;
    }

    if( daemon_fd != -1 ) {
        close(daemon_fd);
        daemon_fd = -1;
    }
}

/* ------------------------------------------------------------------------- *
 * Test Cases
 * ------------------------------------------------------------------------- */

START_TEST(test_message)
{
    const uint32_t id = 42;
    const int extra = 0;

    dsmemsg_generic_t *msg = dsmemsg_new(id, sizeof *msg, sizeof extra);

    ck_assert(msg != NULL);
    ck_assert_int_eq(id, dsmemsg_id(msg));
    ck_assert_int_eq(dsmemsg_size(msg), sizeof *msg);
    ck_assert_int_eq(dsmemsg_extra_size(msg), sizeof extra);
    ck_assert_int_eq(dsmemsg_line_size(msg), sizeof *msg + sizeof extra);

    free(msg);
}
END_TEST

START_TEST(test_send_receive)
{
    dsmesock_connection_t *connection = dsmesock_connect();
    ck_assert(connection != NULL);

    DSM_MSGTYPE_STATE_QUERY msg = DSME_MSG_INIT(DSM_MSGTYPE_STATE_QUERY);
    log_notice("TEST: send(%s)", dsmemsg_name((dsmemsg_generic_t *)&msg));
    dsmesock_broadcast(&msg);

    ck_assert(wait_input(connection->fd) == 1);

    dsmemsg_generic_t *reply = dsmesock_receive(connection);
    ck_assert(reply != NULL);
    log_notice("TEST: recv(%s)", dsmemsg_name(reply));
    ck_assert_int_eq(dsmemsg_id(reply),
                     DSME_MSG_ID_(DSM_MSGTYPE_STATE_REQ_DENIED_IND));
    ck_assert_int_eq(dsmemsg_extra_size(reply), sizeof mock_extra);
    DSM_MSGTYPE_STATE_REQ_DENIED_IND *state_reply =
        DSMEMSG_CAST(DSM_MSGTYPE_STATE_REQ_DENIED_IND, reply);
    ck_assert(state_reply != NULL);
    ck_assert_int_eq(state_reply->state, DSME_STATE_TEST);
    const char *extra = DSMEMSG_EXTRA(state_reply);
    ck_assert(extra != NULL);
    size_t length = strnlen(extra, sizeof mock_extra);
    ck_assert(length < sizeof mock_extra);
    ck_assert(strcmp(extra, mock_extra) == 0);

    free(reply);

    dsmesock_close(connection);
}
END_TEST

static Suite *libdsme_suite(void)
{
    Suite *suite = suite_create("libdsme");

    TCase *testcase = tcase_create("libdsme");

    tcase_add_test(testcase, test_message);
    tcase_add_test(testcase, test_send_receive);

    suite_add_tcase(suite, testcase);

    return suite;
}

/* ------------------------------------------------------------------------- *
 * Test Application
 * ------------------------------------------------------------------------- */

int main(int argc, char **argv)
{
    int exit_code = EXIT_FAILURE;
    const char *output = "/tmp/result.xml";

    /* Handle command line options */
    static const struct option optL[] = {
        {"help",    no_argument ,      0,  'h' },
        {"verbose", no_argument,       0,  'v' },
        {"quiet",   no_argument,       0,  'q' },
        {"output",  required_argument, 0,  'o' },
        {0,         0,                 0,   0  }
    };
    static const char optS[] = "hvqo:";

    for( ;; ) {
        int opt = getopt_long(argc, argv, optS, optL, 0);
        if( opt == -1 )
            break;

        switch( opt ) {
        case 'h':
            printf("Usage:\n"
                   "    %s [options]\n"
                   "\n"
                   "Options:\n"
                   "  -h/--help          Print this help text\n"
                   "  -v/--verbose       Make output one step more verbose\n"
                   "  -q/--quiet         Make output one step less verbose\n"
                   "  -o/--output=<path> Save results to path (default: %s)\n"
                   "\n",
                   *argv, output);
            exit(EXIT_SUCCESS);

        case 'v':
            ++log_level;
            break;

        case 'q':
            --log_level;
            break;

        case 'o':
            output = optarg;
            break;

        case '?':
            /* getopt has already written a diagnostic message */
            goto bailout;

        default:
            fprintf(stderr, "Unhandled option %d '-%c'\n",
                    opt, isalnum(opt) ? opt : '?');
            goto bailout;
        }
    }

    /* Make dsmesock_connect() talk to mock daemon */
    setenv("DSME_SOCKFILE", mock_socket, 1);

    /* Start mock daemon child process */
    if( !daemon_start() )
        goto bailout;

    /* Run the tests */
    SRunner *runner = srunner_create(libdsme_suite());
    srunner_set_xml(runner, output);
    srunner_run_all(runner, CK_NORMAL);
    if( srunner_ntests_failed(runner) == 0 )
        exit_code = EXIT_SUCCESS;
    srunner_free(runner);

bailout:
    /* Terminate mock daemon child process */
    daemon_stop();

    return exit_code;
}
