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
#include "process.h"
#include "setup_seccomp.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include <sched.h>
#include <signal.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <grp.h>
#include <sys/prctl.h>
#include <sys/types.h>
#include <sys/capability.h>


/**
 * Wrapper for system clone function.
 */
pid_t saferun_clone(int (*fn)(void *), void *arg, int flags)
{
    long stack_size = sysconf(_SC_PAGESIZE);
    void *stack = alloca(stack_size) + stack_size;
    pid_t ret;

#ifdef __ia64__
    ret = __clone2(fn, stack,
            stack_size, flags | SIGCHLD, arg);
#else
    ret = clone(fn, stack, flags | SIGCHLD, arg);
#endif

    return ret;
}

/* Set close-on-exec flag to all fd's except 0, 1, 2. (stdin, stdout, stderr) */
void setup_inherited_fds()
{
    struct dirent dirent, *direntp;
    DIR *dir;

    dir = opendir("/proc/self/fd");
    if (!dir) {
        SYSWARN("failed to open dir /proc/self/fd/");
        WARN("can`t check for inherited fds");
        return; //Not a critical error
    }

    int fddir = dirfd(dir);
    while (!readdir_r(dir, &dirent, &direntp)) {
        if (!direntp)
            break;
        if (!strcmp(direntp->d_name, "."))
            continue;
        if (!strcmp(direntp->d_name, ".."))
            continue;

        int fd = atoi(direntp->d_name);
        if (fd == 0 || fd == 1 || fd == 2 || fd == fddir)
            continue;

        /* found inherited fd, setting FD_CLOEXEC */
        if (fcntl(fd, F_SETFD, FD_CLOEXEC) == -1)
            SYSWARN("can`t close-on-exec on fd %d", fd); //not a critical error
    }

    if (closedir(dir))
        SYSWARN("failed to close /proc/self/fd directory");
}

// All output that went fd now goes to to_fd
void redirect_fd(int fd, int to_fd)
{
    if (fd == to_fd || fd < 0)
        return;

    if (close(fd) < 0) {
        SYSERROR("Can`t close old fd: %d", fd);
        abort();
    }

    if (dup2(to_fd, fd) == -1) {
        SYSERROR("can`t redirect fd %d to %d", fd, to_fd);
        abort();
    }
}

void redirect_to_file(int fd, char *filename, const char *mode) {
	if (!filename)
		return;

	FILE * f = fopen(filename, mode);
	if (!f) {
		SYSERROR("Can't open file ""%s""", filename);
		abort();
	}

	redirect_fd(fd, fileno(f));
}

void drop_capabilities() {
    cap_t empty;
    empty = cap_init();
    if ( cap_set_proc(empty) ) {
            SYSERROR("cap_set_proc() failed");
            abort();
    }
    cap_free(empty);
    TRACE("capabilities has been dropped");
}

void do_chroot(const char *dir) {
    if (!dir) return;
    if (chroot(dir) == -1) {
        SYSERROR("can`t chroot");
        abort();
    }
}

void do_chdir(const char *dir) {
    if (!dir) return;
    if (chdir(dir) == -1) {
        SYSERROR("can`t chdir");
        abort();
    }
}

/* Drop uid and gid back to real caller's */
void setup_uidgid() {
	gid_t gid = getgid();
	uid_t uid = getuid();

    // if we set uid first, we wouldn't have rights for setting gid
	if (setgid(gid) == -1 || setuid(uid) == -1) {
        ERROR("Can't set uid and gid");
        abort();
	}
}


int do_start(void *_data) {
	process_t *proc = (process_t *) _data;

	//Setup child after exec.
    prctl(PR_SET_PDEATHSIG, SIGKILL); //child MUST be killed when parent dies
    setup_inherited_fds();

    //Go to jail
    do_chroot(proc->jail.chroot);

    //Set up limits
    //TODO: setup rlimit

    // We should chdir and redirect fd's before dropping capabilities because we may not be able to open files. See issue #1.
    do_chdir(proc->jail.chdir);
    redirect_to_file(STDIN_FILENO, proc->redirect_stdin, "r");
    redirect_to_file(STDOUT_FILENO, proc->redirect_stdout, "w");
    redirect_to_file(STDERR_FILENO, proc->redirect_stderr, "w");

    //Drop all privileges
    setup_uidgid();
    drop_capabilities();

    if (prctl(PR_SET_NO_NEW_PRIVS, 1) == -1)
    	SYSWARN("Can't set NO_NEW_PRIVS flag for the process");

    if (proc->use_seccomp)
        setup_seccomp();

    execvp(proc->argv[0], proc->argv);
    ERROR("Can`t exec %s: %s", proc->argv[0], strerror(errno));
    return 1;
}


int spawn_process(process_t *proc) {
	int clone_flags = 0;
	if (proc->use_namespaces)
		clone_flags = CLONE_NEWUTS | CLONE_NEWPID | CLONE_NEWIPC | CLONE_NEWNET;

    proc->pid = saferun_clone(do_start, proc, clone_flags);

    if (proc->pid < 0) {
        SYSERROR("Failed to clone");
        return -1;
    }

    return 0;
}


