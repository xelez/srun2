all : srun2 suid
srun2 : main.cpp hypervisor.o parser.o log.o profiling.o setup_seccomp.o spawn.o
	g++ main.cpp hypervisor.o parser.o log.o profiling.o setup_seccomp.o spawn.o -lseccomp -lcap ${CPPFLAGS} -o srun2
suid : srun2
	chown root srun2
	chmod u+s srun2
clean :
	rm -f srun2 \
		hypervisor.o \
		parser.o \
		log.o \
		profiling.o \
		setup_seccomp.o \
		spawn.o

hypervisor.o : hypervisor.cpp
	g++ hypervisor.cpp ${CPPFLAGS} -c -o hypervisor.o
parser.o : parser.cpp
	g++ parser.cpp ${CPPFLAGS} -c -o parser.o
log.o : log.cpp
	g++ log.cpp ${CPPFLAGS} -c -o log.o
profiling.o : profiling.cpp
	g++ profiling.cpp ${CPPFLAGS} -c -o profiling.o
setup_seccomp.o : setup_seccomp.cpp
	g++ setup_seccomp.cpp ${CPPFLAGS} -c -o setup_seccomp.o
spawn.o : spawn.cpp
	g++ spawn.cpp ${CPPFLAGS} -c -o spawn.o
.PHONY : all clean suid
