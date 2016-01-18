#include "pm.h"

#include <minix/callnr.h>
#include <minix/com.h>
#include <minix/config.h>
#include <minix/ipc.h>
#include <minix/endpoint.h>
#include <minix/sysutil.h>
#include <minix/syslib.h>
#include <minix/const.h>
#include <minix/type.h>
#include <minix/ds.h>
#include <minix/rs.h>

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <lib.h>

#include "mproc.h"

int do_getsemgroup(void) {
	// TODO: make sure endpoint is IPC server
	pid_t pid = m_in.m1_i1;
	struct mproc *rmp = find_proc(pid);
    int group = rmp->mp_sem_group;
    return group;
}

int do_setsemgroup(void) {
	// TODO: make sure endpoint is an IPC server
    pid_t pid = m_in.m1_i1;
    int new_group = m_in.m1_i2;
	struct mproc *rmp = find_proc(pid);
	rmp->mp_sem_group = new_group;
    return 0;
}

// Copied from user space library
int minix_rs_lookup(const char *name, endpoint_t *value)
{
	message m;
	size_t len_key;

	len_key = strlen(name)+1;

	m.RS_NAME = (char *) __UNCONST(name);
	m.RS_NAME_LEN = len_key;

	if (_syscall(RS_PROC_NR, RS_LOOKUP, &m) != -1) {
		*value = m.RS_ENDPOINT;
		return OK;
	}

	return -1;
}

static void notify_ipc_proc(endpoint_t pt, int group, int type) {
	endpoint_t ipc;
	if (minix_rs_lookup("ipc", &ipc) == OK) {
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
