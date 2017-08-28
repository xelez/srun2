import os
import sys
import re
import subprocess

def get_source():
    dir_path = os.path.dirname(os.path.realpath(__file__))
    setup_seccomp_path = os.path.join(dir_path, '../src/setup_seccomp.cpp')

    with open(setup_seccomp_path) as f:
        return f.read()

def get_saferun_syscalls():
    src = get_source()
    p = re.compile('ALLOW_SYSCALL\((.*)\)')
    return p.findall(src)

def get_strace_output(args):
    result = subprocess.run(['strace'] + args, stderr=subprocess.PIPE, encoding='utf-8')
    return result.stderr

def get_strace_syscalls(args):
    report = get_strace_output(args)
    lines = report.strip().split()
    p = re.compile('([a-z0-9]*)\(.*')
    return [m.group(1) for m in map(p.match, lines) if m is not None and m.group(1)]

saferun_syscalls = set(get_saferun_syscalls())
strace_syscalls = set(get_strace_syscalls(sys.argv[1:]))
print('--- Missing syscalls in saferun: ---')
print('\n'.join(sorted(strace_syscalls - saferun_syscalls)))
