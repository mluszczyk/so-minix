#include "inc.h"
#include <lib.h>

int getsemgroup(pid_t proc_num) {
    message m;
    m.m1_i1 = proc_num;
    int num = _syscall(PM_PROC_NR, GETSEMGROUP, &m);
    return num;
}

int setsemgroup(int group) {
    message m;
    m.m1_i1 = group;
    return _syscall(PM_PROC_NR, SETSEMGROUP, &m);
}


int do_proc_sem_init(message *mess) {
	return -1;
}

int do_proc_sem_post(message *mess) {
	return -1;
}

int do_proc_sem_wait(message *mess) {
	return -1;
}

int do_proc_sem_get_num(message *mess) {
	pid_t proc_num = getnpid(mess->m_source);
	int num = getsemgroup(proc_num);
	printf("ICP: Get num %d -> %d\n", proc_num, num);
	return num;
}
