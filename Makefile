all : suid_srun2 suid_env_helper

srun2 : src/main.cpp src/hypervisor.cpp src/parser.cpp src/log.cpp src/profiling.cpp src/setup_seccomp.cpp src/spawn.cpp
	g++ -O3 -DNDEBUG src/*.cpp -lseccomp -lcap -o srun2

env_helper: helpers/env_helper.cpp
	g++ -O3 helpers/env_helper.cpp -o env_helper

suid_srun2 : srun2
	sudo chown root srun2
	sudo chmod u+s srun2

suid_env_helper: env_helper
	sudo chown root env_helper
	sudo chmod u+s env_helper

clean :
	rm -f srun2


.PHONY : all clean suid
