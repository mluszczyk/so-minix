#include "inc.h"
#include <lib.h>

// Number of groups
#define NR_SEM_GROUPS NR_PROCS
// Group reserved for unused slots in sem_groups table
#define GROUP_NUM_NOT_USED 0
// Default group before calling init
#define GROUP_NUM_NONE -1

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
	int proc_count;
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

void decrease_proc_count(struct sem_group* sg) {
	sg->proc_count -= 1;
	if (sg->proc_count == 0) {
		sg->group_num = GROUP_NUM_NOT_USED;
		sg->sem_count = 0;
		free(sg->sems);
		sg->sems = NULL;
	}
}

// Returns sem_group with given number of NULL if not found.
struct sem_group *find_sem_group(int group_num) {
	for (int i = 0; i < NR_SEM_GROUPS; ++i) {
		if (sem_groups[i].group_num == group_num) {
			return &sem_groups[i];
		}
	}
	return NULL;
}

// Returns unused sem_group or NONE
struct sem_group *get_free_sem_group() {
	return find_sem_group(GROUP_NUM_NOT_USED);
}

// Returns sem_group of given process by PID
struct sem_group *get_sem_group_by_pid(pid_t pid) {
	int group_num = getsemgroup(pid);
	if (group_num == GROUP_NUM_NONE) {
		return NULL;
	} else {
		return find_sem_group(group_num);
	}
}

// Returns sem_group of given process by endpoint
struct sem_group *get_sem_group(endpoint_t endpoint) {
	pid_t pid = getnpid(endpoint);
	return get_sem_group_by_pid(pid);
}

int do_proc_sem_init(message *mess) {
	pid_t pid = getnpid(mess->m_source);
	int count = mess->m1_i1;

	struct sem_group *sg = get_free_sem_group();
	if (sg == NULL) {
		return ENOMEM;
	}
	sg->sems = calloc(count, sizeof(struct semaphore));
	if (sg->sems == NULL) {
		return ENOMEM;
	}

	// from now on it shouldn't fail
	struct sem_group *old_group = get_sem_group_by_pid(pid);
	if (old_group != NULL) {
		decrease_proc_count(old_group);
	}

	int group = next_sem_group;
	++next_sem_group;
	setsemgroup(pid, group);

	memset(sg->sems, 0, count * sizeof(struct semaphore));
	sg->group_num = group;
	sg->sem_count = count;
	sg->proc_count = 1;

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

void proc_forked(endpoint_t pt, int group) {
	if (group == GROUP_NUM_NONE) {
		return;
	}
	struct sem_group *sg = find_sem_group(group);
	if (sg == NULL) {
		return;
	}
	sg->proc_count += 1;
}

void proc_exited(endpoint_t pt, int group) {
	if (group == GROUP_NUM_NONE) {
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
	decrease_proc_count(sg);
}
