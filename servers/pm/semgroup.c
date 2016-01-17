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

// Hard to get minix_rs_lookup working here. -1 means not found
endpoint_t find_ipc() {
	for (struct mproc *rmp = &mproc[0]; rmp < &mproc[NR_PROCS]; ++rmp) {
		if (strcmp(rmp->mp_name, "ipc") == 0) {
			return rmp->mp_endpoint;
		}
	}
	return -1;
}

static void notify_ipc_proc(endpoint_t pt, int group, int type) {
	endpoint_t ipc = find_ipc();
	if (ipc != -1) {
		message m;
		m.m_type = type;
		m.m1_i1 = pt;
		m.m1_i2 = group;
		asynsend3(ipc, &m, AMF_NOREPLY);
	} else {
		printf("IPC not found!\n");
	}
}

void notify_ipc_proc_exit(endpoint_t pt, int group) {
	notify_ipc_proc(pt, group, PM_IPC_PROC_EXITED);
}

void notify_ipc_proc_fork(endpoint_t pt, int group) {
	notify_ipc_proc(pt, group, PM_IPC_PROC_FORKED);
}
