#include <proc_sem.h>
#include <errno.h>
#include <lib.h>
#include <unistd.h>
#include <minix/rs.h>
#include <minix/syslib.h>

static int _ipc_syscall(int param, message *m) {
	endpoint_t ipc_ep;
	minix_rs_lookup("ipc", &ipc_ep);

	return _syscall(ipc_ep, param, m);
}

int proc_sem_get_num(void) {
    message m;
	return _ipc_syscall(IPC_PROC_SEM_GET_NUM, &m);
}

int proc_sem_init(size_t n) {
    message m;
	m.m1_i1 = n;
	int ret = _ipc_syscall(IPC_PROC_SEM_INIT, &m);
	if (ret == -1) {
		return errno;
	} else {
		return ret;
	}
}

void proc_sem_post(size_t sem_nr) {
    message m;
	m.m1_i1 = sem_nr;
	_ipc_syscall(IPC_PROC_SEM_POST, &m);
}

void proc_sem_wait(size_t sem_nr) {
    message m;
	m.m1_i1 = sem_nr;
	_ipc_syscall(IPC_PROC_SEM_WAIT, &m);
}
