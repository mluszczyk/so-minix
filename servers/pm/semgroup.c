#include <stdio.h>
#include <string.h>
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

// Hard to get minix_rs_lookup working here
endpoint_t find_ipc() {
	for (struct mproc *rmp = &mproc[0]; rmp < &mproc[NR_PROCS]; ++rmp) {
		if (strcmp(rmp->mp_name, "ipc") == 0) {
			return rmp->mp_endpoint;
		}
	}
	return 0;
}

void notify_ipc_proc_exit(endpoint_t pt, int group) {
	endpoint_t ipc = find_ipc();
	message m;
	m.m_type = PM_IPC_PROC_EXITED;
	m.m1_i1 = pt;
	m.m1_i2 = group;
	sendnb(ipc, &m);
}
