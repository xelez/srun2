#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mount.h>
#include <unistd.h>
#include <limits.h>
#include <errno.h>

void help(char *cmd) {
    printf("Usage:\n");
    printf("    %s create env_path\n", cmd);
    printf("    %s remove env_path\n\n", cmd);

    printf("Creates or removes bind-mounted chroot.\n\n");
    printf("Expected to be run as setuid-root program.\n");
}

const int MOUNT_DIRS_COUNT = 4;
const char *MOUNT_DIRS[MOUNT_DIRS_COUNT] = {"/usr", "/lib", "/lib64", "/bin"};

uid_t ruid;
uid_t euid;

void init_suid() {
    ruid = getuid ();
    euid = geteuid ();
}

void do_suid()
{
    if (seteuid (euid)) {
        perror("Couldn't set euid");
        exit(-1);
    }
}

void undo_suid()
{
    if (seteuid (ruid)) {
        perror("Couldn't set euid");
        exit(-1);
    }
}

bool directory_exists(const char * dir) {
    struct stat buf;
    return stat(dir, &buf) == 0 && S_ISDIR(buf.st_mode);
}

void bind_dir(const char *path_to_env, const char *dir, bool readonly = true) {
    int full_path_len = strlen(path_to_env) + strlen(dir) + 2;
    char *full_path = (char *) malloc(full_path_len);
    snprintf(full_path, full_path_len, "%s/%s", path_to_env, dir);

    if (mkdir(full_path, 0777)) {
        fprintf(stderr, "Error on creating %s: %s\n", full_path, strerror(errno));
        exit(1);
    }

    unsigned long flags = MS_BIND | MS_NOSUID;
    if (readonly)
        flags |= MS_RDONLY;

    do_suid();
    if (mount(dir, full_path, NULL, flags, NULL)) {
        fprintf(stderr, "Error on mounting %s: %s\n", full_path, strerror(errno));
        exit(1);
    }

    if (readonly) {
        if (mount("none", full_path, NULL, MS_REMOUNT | flags, NULL)) {
            fprintf(stderr, "Error on read-only remounting %s: %s\n", full_path, strerror(errno));
            exit(1);
        }
    }
    undo_suid();

    free(full_path);
}

void unbind_dir(const char *path_to_env, const char *dir, bool readonly = true) {
    int full_path_len = strlen(path_to_env) + strlen(dir) + 2;
    char *full_path = (char *) malloc(full_path_len);
    snprintf(full_path, full_path_len, "%s/%s", path_to_env, dir);

    do_suid();
    if (umount2(full_path, MNT_FORCE | UMOUNT_NOFOLLOW)) {
        fprintf(stderr, "Error on unmounting %s: %s\n", full_path, strerror(errno));
        exit(1);
    }
    undo_suid();

    if (rmdir(full_path)) {
        fprintf(stderr, "Error on removing dir %s: %s\n", full_path, strerror(errno));
        exit(1);
    }

    free(full_path);
}


void create_env(char *path_to_env) {
    if (mkdir(path_to_env, 0777)) {
        if (errno != EEXIST) {
            perror("Can't create directory for new env");
            exit(1);
        }
    }

    for (int i = 0; i < MOUNT_DIRS_COUNT; ++i)
        if (directory_exists(MOUNT_DIRS[i]))
            bind_dir(path_to_env, MOUNT_DIRS[i]);
}

void remove_env(char *path_to_env) {
    for (int i = 0; i < MOUNT_DIRS_COUNT; ++i)
        unbind_dir(path_to_env, MOUNT_DIRS[i]);
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        help(argv[0]);
        return 1;
    }

    init_suid();
    undo_suid();

    //char *path_to_env = realpath(argv[2], NULL);
    //TODO: sanitize path?
    char *path_to_env = argv[2];

    if (path_to_env == NULL) {
        perror("kok");
    }

    if (!strcmp(argv[1], "create")) {
        create_env(path_to_env);
    }
    else if (!strcmp(argv[1], "remove")) {
        remove_env(path_to_env);
    } else {
        fprintf(stderr, "Unknown command: %s\n", argv[1]);
        return 1;
    }

    return 0;
}
