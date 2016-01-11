#include <sys/types.h>

int proc_sem_init(size_t n);
void proc_sem_post(size_t sem_nr);
void proc_sem_wait(size_t sem_nr);
int proc_sem_get_num(void);
