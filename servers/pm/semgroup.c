#include <stdio.h>
#include "pm.h"
#include "mproc.h"

int do_getsemgroup(void) {
	// TODO: make sure endpoint is IPC server
	pid_t pid = m_in.m1_i1;
	struct mproc *rmp = find_proc(pid);
    int group = rmp->mp_sem_group;
    printf("DEBUG: getsemgroup of %d -> %d.\n", pid, group);
    return group;
}

int do_setsemgroup(void) {
	// TODO: make sure endpoint is an IPC server
    pid_t pid = m_in.m1_i1;
    int new_group = m_in.m1_i2;
    printf("DEBUG: setsemgroup of %d to %d.\n", pid, new_group);
	struct mproc *rmp = find_proc(pid);
	rmp->mp_sem_group = new_group;
    return 0;
}
