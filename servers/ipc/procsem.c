#include "inc.h"
#include <lib.h>

#define NR_SEM_GROUPS NR_PROCS
#define GROUP_NUM_NOT_USED 0

// Semaphore operations including data structures are loosely based
// on IPC semaphores.

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

struct sem_group sem_groups[NR_SEM_GROUPS];
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
	for (int i = 0; i < NR_SEM_GROUPS; ++i) {
		if (sem_groups[i].group_num == group_num) {
			return &sem_groups[i];
		}
	}
	// so bad. TODO: deal with it
	printf("Not found!\n");
	return NULL;
}

struct sem_group *get_free_sem_group() {
	return find_sem_group(GROUP_NUM_NOT_USED);
}

struct sem_group *get_sem_group_by_pid(pid_t pid) {
	int group_num = getsemgroup(pid);
	return find_sem_group(group_num);
}

struct sem_group *get_sem_group(endpoint_t endpoint) {
	pid_t pid = getnpid(endpoint);
	return get_sem_group_by_pid(pid);
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
	if (sg == NULL) {
		printf("No group found.\n");
		// TODO: handle this
		return 0;
	}
	sg->group_num = group;
	sg->sem_count = count;
	sg->sems = calloc(count, sizeof(struct semaphore));
	// TODO: it could fail!
	memset(sg->sems, 0, count * sizeof(struct semaphore));

	return 0;
}

static void push_waiting(struct semaphore *sem, endpoint_t endpoint) {
	++sem->waiting_count;
	sem->waiting = realloc(
		sem->waiting, sizeof(endpoint_t) * sem->waiting_count);
	// TODO: what if realloc fails?
	sem->waiting[sem->waiting_count - 1] = endpoint;
}

static void remove_waiting(struct semaphore *sem, int k) {
	--sem->waiting_count;
	memmove(sem->waiting + k, sem->waiting + k + 1,
		(sem->waiting_count - k) * sizeof(endpoint_t));
	sem->waiting = realloc(
		sem->waiting, sizeof(endpoint_t) * sem->waiting_count);
	// TODO: what if realloc fails?
}

static endpoint_t pop_waiting(struct semaphore *sem) {
	endpoint_t endpoint = sem->waiting[0];
	remove_waiting(sem, 0);
	return endpoint;
}

static void wake_process(endpoint_t endpoint) {
	message m;
	m.m_type = OK;
	sendnb(endpoint, &m);
}

int do_proc_sem_post(message *mess) {
	struct sem_group *sg = get_sem_group(mess->m_source);
	if (sg == NULL) {
		// TODO: handle this
		printf("Sem group not found\n");
		return -1;
	}

	size_t num = mess->m1_i1;  // TODO: check if fits array bounds
	struct semaphore *sem = &sg->sems[num];

	if (sem->waiting_count == 0) {
		printf("Endpoint %d increments sem %d from %d\n",
			mess->m_source, num, sem->val);
		++sem->val;
	} else {
		endpoint_t endpoint = pop_waiting(sem);
		printf("Endpoint %d wakes up process %d\n", mess->m_source, endpoint);
		wake_process(endpoint);
	}

	return 0;
}

int do_proc_sem_wait(message *mess) {
	struct sem_group *sg = get_sem_group(mess->m_source);
	size_t num = mess->m1_i1;  // TODO: check if in bounds
	struct semaphore *sem = &sg->sems[num];

	if (sem->val > 0) {
		printf("Endpoint %d going through sem %d of (previous) value %d\n",
			mess->m_source, num, sem->val);
		--sem->val;
		wake_process(mess->m_source);
	} else {
		printf("Endpoint %d waiting for sem %d of value %d\n",
			mess->m_source, num, sem->val);
		push_waiting(sem, mess->m_source);
	}
	return 0;
}

int do_proc_sem_get_num(message *mess) {
	pid_t pid = getnpid(mess->m_source);
	int num = getsemgroup(pid);
	return num;
}

void proc_exited(endpoint_t pt, int group) {
	if (group == -1) {
		return;
	}
	printf("Removing proc %d from group %d\n", pt, group);
	struct sem_group *sg = find_sem_group(group);
	if (sg == NULL) {
		printf("Sem group not found!\n");
		return;
	}
	for (int j = 0; j < sg->sem_count; ++j) {
		struct semaphore *sem = &sg->sems[j];
		for (int k = 0; k < sem->waiting_count; ++k) {
			if (sem->waiting[k] == pt) {
				printf("Removing endpoint %d from waiting\n", pt);
				remove_waiting(sem, k);
				// it shouldn't occur more than once, but 
				// it doesn't cost much to verify the rest
				k--;
			}
		}
	}
}
