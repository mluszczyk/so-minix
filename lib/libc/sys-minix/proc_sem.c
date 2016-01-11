#include <proc_sem.h>
#include <lib.h>
#include <minix/rs.h>
#include <unistd.h>

int proc_sem_get_num(void) {
    message m;
	endpoint_t ipc_ep;
	minix_rs_lookup("ipc", &ipc_ep);

	int num = _syscall(ipc_ep, IPC_PROC_SEM_GET_NUM, &m);

    return num;
}

int proc_sem_init(size_t n) {
    message m;
	m.m1_i1 = n;
	endpoint_t ipc_ep;
	minix_rs_lookup("ipc", &ipc_ep);

	int num = _syscall(ipc_ep, IPC_PROC_SEM_INIT, &m);

	return num;
}
