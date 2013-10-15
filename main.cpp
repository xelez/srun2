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

#include "process.h"
#include "parser.h"
#include "log.h"

#include <stdio.h>
#include <stdlib.h>

static process_t proc;

static parser_option_t options[] = {
    { "--chdir",    "-d", PARSER_ARG_STR,  &proc.jail.chdir,       "Change directory to dir (done after chroot)" },
    { "--chroot",   "-c", PARSER_ARG_STR,  &proc.jail.chroot,      "Do a chroot"},
    { "--hostname", "-h", PARSER_ARG_STR,  &proc.jail.hostname,    "Change hostname"},
    { "--mem",      "-m", PARSER_ARG_INT,  &proc.limits.mem,       "Limit memory usage (in Mbytes)"},
    { "--time",     "-t", PARSER_ARG_INT,  &proc.limits.time,      "Limit user+system execution time (in ms)"},
    { "--real_time","-r", PARSER_ARG_INT,  &proc.limits.real_time, "Limit real execution time (in ms)"},
    { "--seccomp",  "-s", PARSER_ARG_BOOL, &proc.use_seccomp,      "Use seccomp to ensure security"},
    { NULL }
};

void help_and_exit(char *cmd) {
	fprintf(stderr, "Usage: %s [options] [--] command [arg1 arg2 ...]\n", cmd);
	parser_print_help(options);
	exit(1);
}

void set_default_options(process_t *proc) {
    proc->jail.chdir = NULL;
    proc->jail.chroot = NULL;
    proc->jail.hostname = NULL;
    
    proc->limits.mem = 100*1024; // 100 Mbytes
    proc->limits.real_time = 4000; // 4 sec
    proc->limits.time = 2000; // 2 sec

    proc->use_seccomp = false;
    proc->argv = NULL;
}

void run(process_t *proc) {
	spawn_process(proc);
    hypervisor(proc);
}

/* Very important function, also validates security */
int validate_options(process_t *proc) {
	if (proc->limits.mem < 1) {
		ERROR("Memory limit is too small");
		return -1;
	}

	if (proc->limits.real_time < 10) {
		ERROR("Real time limit is too small, must be more than 10 ms");
		return -1;
	}

	if (proc->limits.time < 10) {
		ERROR("Time limit is too small, must be more than 10 ms");
		return -1;
	}

	if (!proc->argv[0]) {
		ERROR("No program to run");
		return -1;
	}

	return 0;
}

int main(int argc, char *argv[]) {
	set_default_options(&proc);

	int idx = parse_options(options, argc, argv);
    if (idx == -1)
    	help_and_exit(argv[0]);
    proc.argv = &argv[idx];

    if (-1 == validate_options(&proc))
    	help_and_exit(argv[0]);

	DEBUG("Current limits:\n"
			  "real time = %d ms\n"
			  "time = %d ms\n"
			  "mem = %d kb\n",
			  proc.limits.real_time,
			  proc.limits.time,
			  proc.limits.mem);

    run(&proc);
    return 0;
}
