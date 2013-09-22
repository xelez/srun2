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

#include "log.h"
#include "utils.h"
#include "options.h"

#include <stdio.h>
#include <stdlib.h>

#include <sys/types.h>
#include <sys/time.h>
#include <sys/prctl.h>
#include <signal.h>
#include <wait.h>
#include <math.h>


void help_and_exit() {
	fprintf(stderr, "Usage: srun2 [options] command [arg1 arg2 ...] \n");
	exit(0);
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

bool is_option(const char *str, const char *short_form, const char *long_form) {
	return !strcmp(str, short_form) || !strcmp(str, long_form);
}

long parse_long_or_crash(const char *str, int min_value, int max_value) {
	long value = strtol(str, NULL, 0);
	if (value < min_value || value > max_value) {
		ERROR("Invalid argument value");
		help_and_exit();
	}
	return value;
}

void parse_options_or_help(process_t *proc, int argc, char *argv[]) {
	int curr_i = 1; // index of current option
	while (curr_i < argc) {
		const char *curr = argv[curr_i++];

		if (is_option(curr, "-m", "--mem"))
			proc->limits.mem = parse_long_or_crash(argv[curr_i++], 1, 1000000000);

		else if (is_option(curr, "-t", "--time"))
			proc->limits.time = parse_long_or_crash(argv[curr_i++], 1, 1000000000);

		else if (is_option(curr, "-r", "--real_time"))
			proc->limits.real_time = parse_long_or_crash(argv[curr_i++], 1, 1000000000);

		else {
			--curr_i;
			break;
		}
	}

	if (curr_i >= argc) {
		ERROR("Nothing to run");
		help_and_exit();
	}

	proc->argv = &argv[curr_i];
}

int main(int argc, char *argv[]) {
	process_t proc;
	set_default_options(&proc);
    parse_options_or_help(&proc, argc, argv);

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
