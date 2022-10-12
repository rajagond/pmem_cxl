#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <libpmemobj.h>

struct my_root {
    int value;
    int is_odd;
};

POBJ_LAYOUT_BEGIN(example);
POBJ_LAYOUT_ROOT(example, struct my_root);
POBJ_LAYOUT_END(example);


int main(int argc, char *argv[]) {
    PMEMobjpool *pop= pmemobj_create("/mnt/pmem/po/pool3.obj", "rweg", 100000000, 0777);

    if(pop == NULL) printf("HI\n");

    TX_BEGIN(pop) {
        TOID(struct my_root) root
            = POBJ_ROOT(pop, struct my_root);
        //adding full root to the transaction
        TX_ADD(root);
        D_RW(root)->value = 4;
        D_RW(root)->is_odd = D_RO(root)->value % 2;
    } TX_END

    TOID(struct my_root) root
            = POBJ_ROOT(pop, struct my_root);
    printf("%d, %d\n", D_RO(root)->value, D_RO(root)->is_odd);
    return 0;
}