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
#include "spawn.h"
#include "hypervisor.h"
#include "log.h"

#include <stdio.h>
#include <stdlib.h>

static process_t proc;
bool output_for_human = false;

static parser_option_t options[] = {
    { "--chdir",    "-d", PARSER_ARG_STR,  &proc.jail.chdir,       "Change directory to dir (done after chroot)" },
    { "--chroot",   "-c", PARSER_ARG_STR,  &proc.jail.chroot,      "Do a chroot"},
    { "--mem",      "-m", PARSER_ARG_INT,  &proc.limits.mem,       "Limit memory usage (in Kbytes)"},
    { "--time",     "-t", PARSER_ARG_INT,  &proc.limits.time,      "Limit user+system execution time (in ms)"},
    { "--real_time","-r", PARSER_ARG_INT,  &proc.limits.real_time, "Limit real execution time (in ms)"},
    { "--seccomp",  "-s", PARSER_ARG_BOOL, &proc.use_seccomp,      "Use seccomp to ensure security"},
    { "--usens",    "-n", PARSER_ARG_BOOL, &proc.use_namespaces,   "Use namespaces to ensure security (add 30ms overhead)"},
    { "--human",    "-h", PARSER_ARG_BOOL, &output_for_human,      "Use human-readable output"},
    { "--redirect-stdin",  "", PARSER_ARG_STR, &proc.redirect_stdin,  "Redirect stdin to file (after chroot and chdir)"},
    { "--redirect-stdout", "", PARSER_ARG_STR, &proc.redirect_stdout, "Redirect stdout to file (after chroot and chdir)"},
    { "--redirect-stderr", "", PARSER_ARG_STR, &proc.redirect_stderr, "Redirect stderr to file (after chroot and chdir)"},
    { NULL }
};

void help_and_exit(char *cmd) {
	fprintf(stderr, "Usage: %s [options] [--] command [arg1 arg2 ...]\n", cmd);
	parser_print_help(options);
	fprintf(stderr, "\nIf --human is not used, then format is:\n");
	fprintf(stderr, "SRUN_REPORT: {string_result} {result} {time} {real_time} {mem} {status} {exit_code_or_string_description_for_signal}\n");
	exit(1);
}

void set_default_options(process_t *proc) {
    proc->jail.chdir = NULL;
    proc->jail.chroot = NULL;
    
    proc->limits.mem = 100*1024; // 100 Mbytes
    proc->limits.real_time = 4000; // 4 sec
    proc->limits.time = 2000; // 2 sec

    proc->redirect_stdin = NULL;
    proc->redirect_stdout = NULL;
    proc->redirect_stderr = NULL;

    proc->use_seccomp = false;
    proc->use_namespaces = true;
    proc->argv = NULL;
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


void print_exit_status(FILE *stream, int status) {
    if (WIFEXITED(status)) {
        fprintf(stream, "exited, status=%d\n", WEXITSTATUS(status));
    } else if (WIFSIGNALED(status)) {
        fprintf(stream, "killed by signal %d = %s\n", WTERMSIG(status), strsignal(WTERMSIG(status)));
    } else if (WIFSTOPPED(status)) {
        fprintf(stream, "stopped by signal %d\n", WSTOPSIG(status));
    } else if (WIFCONTINUED(status)) {
        fprintf(stream, "continued\n");
    }
}

void print_stats_for_human(FILE *stream, process_t *proc) {
	fprintf(stream, "Result:    %10s\n", result_to_str[proc->stats.result]);
	fprintf(stream, "Time:      %10ld (ms)\n", proc->stats.time);
	fprintf(stream, "Real Time: %10ld (ms)\n", proc->stats.real_time);
	fprintf(stream, "Memory:    %10ld (kB)\n", proc->stats.mem);
	fprintf(stream, "Status:  ");
	print_exit_status(stream, proc->stats.status);
}

void print_stats(FILE *stream, process_t *proc) {
	fprintf(stream, "SRUN_REPORT: %s %d %ld %ld %ld %d ",
			result_to_str[proc->stats.result],
			proc->stats.result,
			proc->stats.time,
			proc->stats.real_time,
			proc->stats.mem,
			proc->stats.status);

	if (WIFEXITED(proc->stats.status))
		fprintf(stream, "%d\n", WEXITSTATUS(proc->stats.status));
	if (WIFSIGNALED(proc->stats.status))
		fprintf(stream, "%s\n", strsignal(WTERMSIG(proc->stats.status)));
}

void run(process_t *proc) {
	if (spawn_process(proc) == -1)
		exit(1);
    hypervisor(proc);
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

    if (output_for_human)
    	print_stats_for_human(stderr, &proc);
    else
    	print_stats(stderr, &proc);

    return 0;
}
