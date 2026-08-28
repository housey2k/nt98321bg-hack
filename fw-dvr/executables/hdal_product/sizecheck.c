#include <stdio.h>
#include <stddef.h>
#include "hdal.h"

int main(void) {
    printf("sizeof(HD_COMMON_MEM_INIT_CONFIG) = %zu (kernel/pif.o expects 5632)\n",
           sizeof(HD_COMMON_MEM_INIT_CONFIG));
    printf("sizeof(COMMON_MEM_POOL_INFO-equivalent pool_info[0]) = %zu\n",
           sizeof(((HD_COMMON_MEM_INIT_CONFIG *)0)->pool_info[0]));
    printf("HD_COMMON_MEM_MAX_POOL_NUM = %d\n", HD_COMMON_MEM_MAX_POOL_NUM);
    printf("offsetof(HD_COMMON_MEM_INIT_CONFIG, pool_info) = %zu\n",
           offsetof(HD_COMMON_MEM_INIT_CONFIG, pool_info));
    return 0;
}
