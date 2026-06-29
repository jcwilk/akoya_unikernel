/*
 * Execute a shell script with CAP_NET_ADMIN raised in the ambient capability set.
 * Installed with: setcap cap_net_admin,cap_chown+ep bridge-cap-exec
 */
#define _GNU_SOURCE

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/capability.h>
#include <sys/prctl.h>

static int raise_cap_ambient(cap_value_t cap) {
    cap_t caps;

    caps = cap_get_proc();
    if (!caps) {
        return -1;
    }

    if (cap_set_flag(caps, CAP_EFFECTIVE, 1, &cap, CAP_SET) != 0
        || cap_set_flag(caps, CAP_PERMITTED, 1, &cap, CAP_SET) != 0
        || cap_set_flag(caps, CAP_INHERITABLE, 1, &cap, CAP_SET) != 0
        || cap_set_proc(caps) != 0) {
        cap_free(caps);
        return -1;
    }
    cap_free(caps);

    if (prctl(PR_CAP_AMBIENT, PR_CAP_AMBIENT_RAISE, cap, 0, 0) != 0) {
        return -1;
    }

    return 0;
}

static int raise_bridge_caps_ambient(void) {
    if (raise_cap_ambient(CAP_NET_ADMIN) != 0) {
        return -1;
    }
    if (raise_cap_ambient(CAP_CHOWN) != 0) {
        return -1;
    }
    return 0;
}

int main(int argc, char **argv) {
    char **bash_argv;
    int i;

    if (argc < 2) {
        fprintf(stderr, "usage: %s SCRIPT [ARGS...]\n", argv[0]);
        return 2;
    }

    if (raise_bridge_caps_ambient() != 0) {
        fprintf(stderr,
                "bridge-cap-exec: failed to raise CAP_NET_ADMIN/CAP_CHOWN "
                "(re-run scripts/install-bridge-libexec.sh as admin)\n");
        return 1;
    }

    bash_argv = calloc((size_t)argc + 1, sizeof(char *));
    if (!bash_argv) {
        fprintf(stderr, "bridge-cap-exec: out of memory\n");
        return 1;
    }

    bash_argv[0] = "/bin/bash";
    for (i = 0; i < argc - 1; i++) {
        bash_argv[i + 1] = argv[i + 1];
    }
    bash_argv[argc] = NULL;

    execv("/bin/bash", bash_argv);
    fprintf(stderr, "bridge-cap-exec: exec bash failed: %s\n", strerror(errno));
    free(bash_argv);
    return 127;
}
