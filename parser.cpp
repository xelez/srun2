/*
 *  Copyright 2013 Alexander Ankudinov
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 */


#include "parser.h"
#include "log.h"

#include <string.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>

parser_option_t *find_option(parser_option_t *options, char *name) {
    parser_option_t * opt = options;
    while (opt->long_name) {
        if ( !strcmp(name, opt->short_name) || !strcmp(name, opt->long_name) )
            return opt;
        ++opt;
    }
    return NULL;
}

int parse_str(parser_option_t *option, char *arg) {
    *((char **)option->variable) = strdup(arg);
    return 0;
}

int parse_int(parser_option_t *option, char *arg) {
    errno = 0;
    char *end;
    int res = strtol(arg, &end, 0);

    if (arg[0] == '\0' || end[0] != '\0') {
        ERROR("Argument for ""%s"" is not a valid integer value",  option->long_name);
        return -1;
    }

    if (res == LONG_MIN || res == LONG_MAX) {
        ERROR("Argument for ""%s"" is outside of integer range", option->long_name);
        return -1;
    }

    *((int *)option->variable) = res;
    return 0;
}

int parse_bool(parser_option_t *option, char *arg) {
    if (!strcmp("1", arg) || !strcmp("on", arg) ) {
        *((bool *)option->variable) = true;
        return 0;
    }

    if (!strcmp("0", arg) || !strcmp("off", arg) ) {
        *((bool *)option->variable) = false;
        return 0;
    }

    ERROR("Unknown argument for ""%s""", option->long_name);
    return -1;
}


int parse_option(parser_option_t *option, char *arg) {
    switch (option->type) {
    case PARSER_ARG_INT:
        return parse_int(option, arg);
    case PARSER_ARG_BOOL:
        return parse_bool(option, arg);
    case PARSER_ARG_STR:
        return parse_str(option, arg);
    default:
        ERROR("Unknown option type for ""%s""", option->long_name);
        return -1;
    }
}

/** @return index of first unparsed element in argv, or -1 on error*/
int parse_options(parser_option_t *options, int argc, char **argv) {
    int idx = 1;

    //after while idx must point to first unparsed element
    while (idx < argc) {
        char *cur = argv[idx];
        char *arg = argv[idx+1]; // argv has extra NULL at the end, so no array overrun

        if (cur[0] != '-') //not an option
            break;

        if ( !strcmp(cur, "--") ) {
            ++idx;
            break;
        }

        parser_option_t *opt = find_option(options, cur);

        if (!opt) {
            ERROR("Unknown option ""%s""", cur);
            return -1;
        }

        if (!arg) {
            ERROR("No argument for option ""%s""", cur);
            return -1;
        }

        if (parse_option(opt, arg))
            return -1;

        idx += 2;
    }

    return idx;
}

int max_option_len(parser_option_t *options) {
    int max_len = 0;

    parser_option_t *opt = options;
    while (opt->long_name) {
        int l = strlen(opt->long_name);
        if (l > max_len)
            max_len = l;
        ++opt;
    }

    return max_len;
}

void parser_print_help(parser_option_t *options) {
    fprintf(stderr, "Options:\n");

    int max_len = max_option_len(options);
    parser_option_t *opt = options;
    while (opt->long_name) {
        fprintf(stderr, "\t%s, %-*s  - %s\n", opt->short_name, max_len, opt->long_name, opt->description);
        ++opt;
    }
}
