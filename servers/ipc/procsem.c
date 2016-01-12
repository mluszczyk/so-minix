#include "inc.h"
#include <lib.h>

#define SEM_GROUPS_MAX 10
#define GROUP_NUM_NOT_USED 0

struct semaphore {
	int val;
	endpoint_t *waiting;
	int waiting_count;
};

struct sem_group {
	int group_num;  // default is 0 = GROUP_NUM_NOT_USED
	struct semaphore *sems;
	int sem_count;
};

struct sem_group sem_groups[SEM_GROUPS_MAX];
static int next_sem_group = 1;

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

struct sem_group *find_sem_group(int group_num) {
	printf("Find sem group %d\n", group_num);
	for (int i = 0; i < SEM_GROUPS_MAX; ++i) {
		if (sem_groups[i].group_num == group_num) {
			printf("Found %d\n", i);
			return &sem_groups[i];
		}
	}
	// so bad. TODO: deal with it
	printf("Not found!\n");
	return NULL;
}

struct sem_group *get_free_sem_group() {
	return find_sem_group(0);
}

struct sem_group *get_sem_group_by_pid(pid_t pid) {
	int group_num = getsemgroup(pid);
	return find_sem_group(group_num);
}

struct sem_group *get_sem_group(endpoint_t endpoint) {
	pid_t pid = getnpid(endpoint);
	int group_num = getsemgroup(pid);
	return find_sem_group(group_num);
}

int do_proc_sem_init(message *mess) {
	pid_t pid = getnpid(mess->m_source);
	int count = mess->m1_i1;
	printf("ICP: Set group of %d to %d (%d)\n",
		pid, next_sem_group, mess->m1_i1);
	int group = next_sem_group;
	++next_sem_group;
	setsemgroup(pid, group);

	struct sem_group *sg = get_free_sem_group();
	sg->group_num = group;
	sg->sem_count = count;
	sg->sems = calloc(count, sizeof(struct semaphore));
	// TODO: it could fail!
	memset(sg->sems, 0, count * sizeof(struct semaphore));

	return 0;
}

int do_proc_sem_post(message *mess) {
	printf("First sem %d\n", sem_groups[0].group_num);
	struct sem_group *sg = get_sem_group(mess->m_source);
	size_t num = mess->m1_i1;  // TODO: check if fits array bounds
	printf("Called do_proc_sem_post sem %d/%d\n", num, sg->sem_count);
	struct semaphore *sem = &sg->sems[num];

	printf("Endpoint %d increments sem %d from %d\n",
		mess->m_source, num, sem->val);
	++sg->sems[num].val;

	// TODO: wake a waiting process
	return 0;
}

int do_proc_sem_wait(message *mess) {
	struct sem_group *sg = get_sem_group(mess->m_source);
	size_t num = mess->m1_i1;  // TODO: check if in bounds
	struct semaphore *sem = &sg->sems[num];

	printf("Endpoint %d waiting for sem %d of value %d\n",
		mess->m_source, num, sem->val);
	if (sem->val > 0) {
		sem->val -= 1;
		message m;
		m.m_type = OK;
		sendnb(mess->m_source, &m);
	} else {
		++sem->waiting_count;
		sem->waiting = realloc(
			sem->waiting, sizeof(endpoint_t) * sem->waiting_count);
	}
	return 0;
}

int do_proc_sem_get_num(message *mess) {
	pid_t pid = getnpid(mess->m_source);
	int num = getsemgroup(pid);
	printf("ICP: Get num %d -> %d\n", pid, num);
	return num;
}
