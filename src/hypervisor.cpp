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
#include "log.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <signal.h>

/* in usecs */
#define HYPERVISOR_DELAY 25*1000

/* Reading from proc constants */
#define PROC_FILENAME_MAX_LEN 50
#define PROC_READ_BUF_SIZE 256

/*Timeval to milliseconds */
#define TV_TO_MSEC(a) ((a).tv_sec * 1000 + (a).tv_usec / 1000)

/** @return real time in milliseconds */
long get_rtime()
{
    struct timeval t;
    gettimeofday(&t, NULL);
    return TV_TO_MSEC(t);
}

/* The code below uses system files:
 *  /proc/<pid>/schedstat - consistent during one read()
 *  /proc/<pid>/stat - seems to be consistent during one read() or even during open()
 *  /proc/<pid>/status - same here
 *
 *  Because of this we need to read whole file with one call.
 *  See http://stackoverflow.com/questions/5713451/is-it-safe-to-parse-a-proc-file
 *  for more information.
 */

int read_from_proc(const char *filename, const pid_t pid, char *buf, const size_t buf_len) {
	char full_fname[PROC_FILENAME_MAX_LEN];
	snprintf(full_fname, PROC_FILENAME_MAX_LEN, "/proc/%d/%s", pid, filename);
	int fd = open(full_fname, O_RDONLY);
	read(fd, buf, buf_len);
	close(fd);
	return 0; //TODO: add error checking
}

/* in microseconds, needs investigation, may be more than ru_utime+ru_stime */
long get_time_from_proc2(const pid_t pid) {
	char buf[PROC_READ_BUF_SIZE];
	read_from_proc("schedstat", pid, buf, PROC_READ_BUF_SIZE);
	long long time;
	sscanf(buf, "%lld", &time);
	return time;
}

/* in milliseconds, precision is 10 ms, but it's enough */
long get_time_from_proc(const pid_t pid) {
	char buf[PROC_READ_BUF_SIZE];
	read_from_proc("stat", pid, buf, PROC_READ_BUF_SIZE);

	unsigned long stime, utime;
	sscanf(buf,
		"%*d %*s %*c %*d %*d " //pid, comm, state, ppid, pgrp
		"%*d %*d %*d %*u "     //session, tty_nr, tpgid, flags
		"%*u %*u %*u %*u "     //minflt, cminflt, majflt, cmajflt
		"%lu %lu", &stime, &utime);

	return (stime + utime)*1000 / sysconf(_SC_CLK_TCK); //return in milliseconds
}

// in KiloBytes
long get_mem_from_proc(const pid_t pid) {
	char buf[512]; //this file is bigger
	read_from_proc("status", pid, buf, 512);

	char * pos = strstr(buf, "VmHWM:");
	if (!pos)
		return 0;

	long mem;
	sscanf(pos, "%*s %ld", &mem);
	return mem;
}

void sigalrm_handler(int sig) {
	TRACE("alarm");
}

void set_sigalrm_handler(sighandler_t handler) {
	struct sigaction act;
 	act.sa_handler = handler;
	act.sa_flags = 0;
	sigemptyset (&act.sa_mask);
	sigaction(SIGALRM, &act, NULL);
}

void set_timeout(long usecs) {
	itimerval new_value;
	new_value.it_interval.tv_sec = 0;
	new_value.it_interval.tv_usec = 0;
	new_value.it_value.tv_sec = 0;
	new_value.it_value.tv_usec = usecs;
	setitimer(ITIMER_REAL, &new_value, NULL);
}

void reset_timeout() {
	set_timeout(0);
	set_timeout(0);
	//Yes, two times, because first call theoretically can be interrupted by a signal
}


void check_exit_status(stats_t *stats, const int status) {
	stats->status = status;
    if (stats->result != _OK)
        return;

    if ( WIFEXITED(status) && WEXITSTATUS(status) )
        stats->result = _RE;
    else if (WIFSIGNALED(status))
    	stats->result = (WTERMSIG(status) == SIGSYS) ? _SV : _RE;
    else
        stats->result = _OK;
}

void check_time(stats_t *stats, const limits_t *limits, const long time) {
    stats->time = time;
    if (stats->result == _OK && time > limits->time)
        stats->result = _TL;
}

void check_rtime(stats_t *stats, const limits_t *limits) {
    stats->real_time = (get_rtime() - stats->start_time);
    if (stats->result == _OK && stats->real_time > limits->real_time)
        stats->result = _TL;
}

void check_memory(stats_t *stats, const limits_t *limits, const long mem) {
	stats->mem = (mem > stats->mem) ? (mem) : (stats->mem);
    if (stats->result == _OK && stats->mem > limits->mem)
    	stats->result = _ML;
}


void hypervisor(process_t *proc) {
    proc->stats.start_time = get_rtime();

    set_sigalrm_handler(sigalrm_handler);

    proc->stats.mem = 0;
    proc->stats.time = 0;
    proc->stats.real_time = 0;

    while(1) {
        set_timeout(HYPERVISOR_DELAY);

    	int status;
    	struct rusage usage;
        pid_t ret = wait4(proc->pid, &status, 0, &usage);

        if (ret == proc->pid) { /* if child terminated */
        	DEBUG("process terminated");
        	reset_timeout();

        	check_rtime(&proc->stats, &proc->limits);
			const long time = TV_TO_MSEC(usage.ru_utime) + TV_TO_MSEC(usage.ru_stime);
			check_time(&proc->stats, &proc->limits, time);
			check_memory(&proc->stats, &proc->limits, usage.ru_maxrss);
			check_exit_status(&proc->stats, status);
			DEBUG("maxrss: %d, rtime: %d, time: %d, result = %d",
					usage.ru_maxrss, proc->stats.real_time, time, proc->stats.result);
            break;
        }

        check_rtime(&proc->stats, &proc->limits);
		check_time(&proc->stats, &proc->limits, get_time_from_proc(proc->pid));
		check_memory(&proc->stats, &proc->limits, get_mem_from_proc(proc->pid));

		TRACE("Current stats:\n"
				  "real time = %d ms\n"
				  "time = %d ms\n"
				  "mem = %d kb\n"
				  "result = %d",
				  proc->stats.real_time,
				  proc->stats.time,
				  proc->stats.mem,
				  proc->stats.result);

        if (proc->stats.result != _OK) //one of the limits exceeded
            kill(proc->pid, SIGKILL);
    }
}
