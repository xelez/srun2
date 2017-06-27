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

#ifndef OPTIONS_H_
#define OPTIONS_H_

#include <unistd.h>

struct limits_t {
    long mem;       /**< Kbytes */
    long time;      /**< milliseconds */
    long real_time; /**< milliseconds */
};

struct jail_t {
    char *chroot;
    char *chdir;
    // TODO
    //int max_files; //see rlimit
    //int max_stack;
    //int max_filesize; + rlimit CPU + rlimit AS
};

enum result_t {
    _OK = 0, /**< Clean exit, no errors */
    _RE = 1, /**< Runtime error */
    _TL = 2, /**< Time limit exceeded */
    _ML = 3, /**< Memory limit exceeded */
    _SV = 4, /**< Security Violation */
    _SC = 5  /**< System crash */
};

const char* const result_to_str[] = {"OK", "RE", "TL", "ML", "SV", "SC"};

/* Run statistics */
struct stats_t {
    long real_time;        /**< milliseconds */
    long time;             /**< milliseconds */
    long mem;              /**< Kbytes */
    long long start_time; /**< milliseconds, since epoch */

    int status; /**< status code, returned by waitpid function, @see man 2 waitpid for details */

    result_t result;
};

struct process_t {
    limits_t limits;
    jail_t jail;
    stats_t stats;

    char *redirect_stdin;
    char *redirect_stdout;
    char *redirect_stderr;

    bool use_seccomp;
    bool use_namespaces;

    char **argv;
    pid_t pid;
};

#endif /* OPTIONS_H_ */
