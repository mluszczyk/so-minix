#include "inc.h"
#include <lib.h>

int getsemgroup(pid_t pid) {
    message m;
    m.m1_i1 = pid;
    int num = _syscall(PM_PROC_NR, GETSEMGROUP, &m);
    return num;
}

int setsemgroup(pid_t pid, int group) {
    message m;
    m.m1_i1 = pid;
    m.m1_i2 = group;
    return _syscall(PM_PROC_NR, SETSEMGROUP, &m);
}

static int next_sem_group = 0;

int do_proc_sem_init(message *mess) {
	/* TODO: Really init the semaphores! */
	pid_t pid = getnpid(mess->m_source);
	printf("ICP: Set group of %d to %d\n", pid, next_sem_group);
	setsemgroup(pid, next_sem_group);
	++next_sem_group;
	return 0;
}

int do_proc_sem_post(message *mess) {
	return -1;
}

int do_proc_sem_wait(message *mess) {
	return -1;
}

int do_proc_sem_get_num(message *mess) {
	pid_t pid = getnpid(mess->m_source);
	int num = getsemgroup(pid);
	printf("ICP: Get num %d -> %d\n", pid, num);
	return num;
}
