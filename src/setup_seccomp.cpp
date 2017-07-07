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

#include <seccomp.h>
#include <stdlib.h>

/*
 * Workes with this rule set:
 *  - simpe C++
 *  - Python 2.7
 */

void setup_seccomp() {
    int ret = -1;
    scmp_filter_ctx ctx;

    ctx = seccomp_init(SCMP_ACT_KILL);
    if (ctx == NULL) goto err;

//For less copypasting:
#define ALLOW_SYSCALL(syscall) \
    ret = seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(syscall), 0); \
    if (ret < 0) goto err;
// end of macros

    // -- ALL RULES GO HERE --

    // File access and file descriptors
    ALLOW_SYSCALL(access);
    ALLOW_SYSCALL(open);
    ALLOW_SYSCALL(read);
    ALLOW_SYSCALL(write);
    ALLOW_SYSCALL(close);
    ALLOW_SYSCALL(fstat);
    ALLOW_SYSCALL(lstat);
    ALLOW_SYSCALL(stat);
    ALLOW_SYSCALL(ioctl);
    ALLOW_SYSCALL(lseek);
    ALLOW_SYSCALL(openat);
    ALLOW_SYSCALL(readlink);
    ALLOW_SYSCALL(getdents);
    ALLOW_SYSCALL(unlink);
    ALLOW_SYSCALL(dup);
    ALLOW_SYSCALL(dup2);
    ALLOW_SYSCALL(dup3);


    // memory
    ALLOW_SYSCALL(brk);
    ALLOW_SYSCALL(mmap);
    ALLOW_SYSCALL(mprotect);
    ALLOW_SYSCALL(munmap);

    // getting info
    ALLOW_SYSCALL(getcwd);
    ALLOW_SYSCALL(getegid);
    ALLOW_SYSCALL(geteuid);
    ALLOW_SYSCALL(getgid);
    ALLOW_SYSCALL(getuid);
    ALLOW_SYSCALL(getrlimit);

    // futexes
    ALLOW_SYSCALL(futex);
    ALLOW_SYSCALL(set_robust_list);

    //signals
    ALLOW_SYSCALL(rt_sigaction);
    ALLOW_SYSCALL(rt_sigprocmask);

    // other
    ALLOW_SYSCALL(execve);
    ALLOW_SYSCALL(exit_group);
    ALLOW_SYSCALL(set_tid_address);
    ALLOW_SYSCALL(arch_prctl);

    // -- end of rules part --

    ret = seccomp_load(ctx);
    if (ret < 0) goto err;

    seccomp_release(ctx);
    return;

err:
    ERROR("Error while installing seccomp_filter");
    seccomp_release(ctx);
    abort();
}

