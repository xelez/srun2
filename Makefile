all : srun2 suid
srun2 : src/main.cpp src/hypervisor.cpp src/parser.cpp src/log.cpp src/profiling.cpp src/setup_seccomp.cpp src/spawn.cpp
	g++ -O3 -DNDEBUG src/*.cpp -lseccomp -lcap -o srun2
suid : srun2
	sudo chown root srun2
	sudo chmod u+s srun2
clean :
	rm -f srun2

.PHONY : all clean suid
