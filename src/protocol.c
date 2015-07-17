/*  Embedis - Embedded Dictionary Server
    Copyright (C) 2015 PatternAgents, LLC

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

/**
  ******************************************************************************
  *
  * @file        protocol.c
  * @version     0.0.1
  * @date        2015-06-29
  * @copyright   PatternAgents, LLC
  * @brief       The Embedis Dictionary Server Protocol
  *
  ******************************************************************************
  */

/// @file

#include "embedis.h"

const char* EMBEDIS_OK = "OK";
const char* EMBEDIS_UNKNOWN_COMMAND = "unknown command";
const char* EMBEDIS_SYNTAX_ERROR = "syntax error";
const char* EMBEDIS_BUFFER_OVERFLOW = "buffer overflow";
const char* EMBEDIS_ARGS_ERROR = "bad argument count";
const char* EMBEDIS_STORAGE_OVERFLOW = "storage overflow";


void embedis_emit_newline() {
    embedis_out('\r');
    embedis_out('\n');
}


void embedis_emit_integer(int i) {
    size_t j, k;
    if (i < 0) {
        i = -i;
        embedis_out('-');
    }
    for (j = 1; i / (j*10); j *= 10) {}
    while (j) {
        k = i / j;
        embedis_out('0' + k);
        i -= k * j;
        j /= 10;
    }
}


void embedis_emit_size(char kind, size_t length) {
    embedis_out(kind);
    embedis_emit_integer(length);
    embedis_emit_newline();
}


void embedis_response_error(const char* message) {
    const char* errstr = "-ERROR";
    if (message == EMBEDIS_OK) {
        embedis_response_simple(message);
        return;
    }
    while(*errstr) {
        embedis_out(*errstr);
        errstr++;
    }
    if (message) {
        embedis_out(' ');
        while(*message) {
            embedis_out(*message);
            message++;
        }
    }
    embedis_emit_newline();
}


void embedis_response_simple(const char* message) {
    embedis_out('+');
    while(*message) {
        embedis_out(*message);
        message++;
    }
    embedis_emit_newline();
}


void embedis_response_string(const char* message, size_t length) {
    size_t i = 0;
    embedis_emit_size('$', length);
    for (i = 0; i < length; i++) {
        embedis_out(message[i]);
    }
    embedis_emit_newline();
}


void embedis_response_null() {
    const char* nullstr = "$-1";
    while(*nullstr) {
        embedis_out(*nullstr);
        nullstr++;
    }
    embedis_emit_newline();
}


typedef struct embedis_prototol_state {
    embedis_state state;
    size_t pos;
    char buf[EMBEDIS_COMMAND_BUF_SIZE];
    char mode;
} embedis_prototol_state;

static embedis_prototol_state command;


// Initialize everything. Call once at program start.
void embedis_init() {
    command.state.dictionary = &embedis_dictionaries[0];
    embedis_reset();
}


// Reset only the protocol state.
// Call this when a UART notifies you of a break condition.
void embedis_reset() {
    command.mode = 0;
    command.state.argc = 0;
    command.state.argv[0] = command.buf;
    command.pos = 0;
}


int embedis_strcmp(const char* s1, const char* s2) {
    int rc;
    while (1) {
        if (!*s1 && !*s2) return 0;
        rc = *s1 - *s2;
        if (rc) return rc;
        s1++;
        s2++;
    }
}


int embedis_stricmp(const char* s1, const char* s2) {
    int rc, us1, us2;
    while (1) {
        if (!*s1 && !*s2) return 0;
        us1 = *s1;
        us2 = *s2;
        if (us1 >= 'a' && us1 <= 'z') us1 -= 32;
        if (us2 >= 'a' && us2 <= 'z') us2 -= 32;
        rc = us1 - us2;
        if (rc) return rc;
        s1++;
        s2++;
    }

}


// Main command dispatcher.
static void embedis_dispatch() {
    const embedis_command* cmd = &embedis_commands[0];
    while (cmd->name) {
        if (!embedis_stricmp(cmd->name, command.state.argv[0])) break;
        cmd++;
    }
    (*cmd->call)(&command.state);
}


// Process characters received over connection.
void embedis_in(char data) {

    if (command.mode == 0) {
        switch (data) {
        case '+':
        case '-':
        case ':':
        case '$':
        case '*':
            //                mode = data; // TODO
            return;
        case '\r':
        case '\n':
            // nop
            return;
        default:
            if (command.pos != 0) {
                embedis_reset();
                embedis_response_error(EMBEDIS_SYNTAX_ERROR);
                return;
            }
            command.mode = ' ';
            break;
        }
    }


    //    if (command.mode == 0 && command.pos == 0) command.mode = ' ';


    if (command.mode == ' ') {
        if (data == '\r' || data == '\n') {
            if (!command.pos) {
                embedis_reset();
                return;
            }
            // Deal with trailing spaces
            if (command.state.argv[command.state.argc] != &command.buf[command.pos]) {
                command.state.argc++;
            } else {
                command.pos--;
            }
            // Check for overflows
            if (command.state.argc > EMBEDIS_COMMAND_MAX_ARGS) {
                embedis_response_error(EMBEDIS_ARGS_ERROR);
                embedis_reset();
                return;
            }
            if (command.pos >= EMBEDIS_COMMAND_BUF_SIZE) {
                embedis_response_error(EMBEDIS_BUFFER_OVERFLOW);
                embedis_reset();
                return;
            }
            command.buf[command.pos] = 0;
            command.pos++;
            command.state.argv[command.state.argc] = &command.buf[command.pos];
            embedis_dispatch();
            embedis_reset();
            return;
        }

        if (data == ' ') {
            if (command.state.argv[command.state.argc] == &command.buf[command.pos]) {
                // drop extra spaces
                return;
            }
            if (command.pos < EMBEDIS_COMMAND_BUF_SIZE) {
                command.buf[command.pos] = 0;
                command.pos++;
            }
            command.state.argc++;
            if (command.state.argc <= EMBEDIS_COMMAND_MAX_ARGS) {
                command.state.argv[command.state.argc] = &command.buf[command.pos];
            }
            return;
        }

        if (command.pos < EMBEDIS_COMMAND_BUF_SIZE) {
            command.buf[command.pos] = data;
            command.pos++;
        }

        return;
    }

}
