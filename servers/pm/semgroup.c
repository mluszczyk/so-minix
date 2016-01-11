#include <stdio.h>
#include "pm.h"
#include "mproc.h"

int do_getsemgroup(void) {
	// TODO: make sure endpoint is IPC server
	pid_t proc_num = m_in.m1_i1;
	struct mproc *rmp = find_proc(proc_num);
    int group = rmp->mp_sem_group;
    printf("DEBUG: getsemgroup of %d -> %d.\n", proc_num, group);
    return group;
}

int do_setsemgroup(void) {
    int new_group = m_in.m1_i1;
    printf("DEBUG: setsemgroup of %d to %d.\n", who_p, new_group);
    mp->mp_sem_group = new_group;
    return 0;
}
