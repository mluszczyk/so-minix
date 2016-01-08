#include <stdio.h>
#include "pm.h"
#include "mproc.h"

int do_getsemgroup(void) {
    int group = mp->mp_sem_group;
    printf("DEBUG: getsemgroup of %d -> %d.\n", who_p, group);
    return group;
}

int do_setsemgroup(void) {
    int new_group = m_in.m1_i1;
    printf("DEBUG: setsemgroup of %d to %d.\n", who_p, new_group);
    mp->mp_sem_group = new_group;
    return 0;
}
