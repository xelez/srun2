all : srun2 suid
srun2 : main.cpp hypervisor.cpp parser.cpp log.cpp profiling.cpp setup_seccomp.cpp spawn.cpp
	g++ -O3 -DNDEBUG *.cpp -lseccomp -lcap -o srun2
suid : srun2
	sudo chown root srun2
	sudo chmod u+s srun2
clean :
	rm -f srun2

.PHONY : all clean suid
