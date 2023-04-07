#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <libpmemobj.h>

#define MAX_BUF_LEN 50

struct my_root {
    int rp;
    int wp;

    int buf[MAX_BUF_LEN];
};

POBJ_LAYOUT_BEGIN(rweg);
POBJ_LAYOUT_ROOT(rweg, struct my_root);
POBJ_LAYOUT_END(rweg);

int main(int argc, char *argv[]) {
    PMEMobjpool *pop= pmemobj_open("/mnt/pmem/po/pool.obj", POBJ_LAYOUT_NAME(rweg));

    if (pop == NULL) {
        pop = pmemobj_create("/mnt/pmem/po/pool.obj", POBJ_LAYOUT_NAME(rweg), (100000000), 0666);
        printf("create done\n");
        //fibonacci
        TX_BEGIN(pop) {
            TOID(struct my_root) root = POBJ_ROOT(pop, struct my_root);

            // adding full root to the transaction
            TX_ADD(root);

            D_RW(root)->rp = 0;
            D_RW(root)->buf[0] = 0;
            D_RW(root)->wp = 1;
        } TX_END
        printf("1 transaction done\n");
        TX_BEGIN(pop) {
            TOID(struct my_root) root = POBJ_ROOT(pop, struct my_root);

            // adding full root to the transaction
            TX_ADD(root);

            D_RW(root)->buf[D_RO(root)->wp] = 1;
            D_RW(root)->wp = D_RO(root)->wp + 1;
        } TX_END
    }

    PMEMoid root = pmemobj_root(pop, sizeof(struct my_root));
    struct my_root *rootp = pmemobj_direct(root);

    int i = rootp->wp;

 
    for (; i < 50; i++)
    {
        sleep(1);
        /* Add the previous 2 numbers in the series and store it */
        TX_BEGIN(pop) {
            TOID(struct my_root) root = POBJ_ROOT(pop, struct my_root);

            // adding full root to the transaction
            TX_ADD(root);
            printf("%d ", D_RO(root)->buf[D_RO(root)->rp]);
            fflush(stdout);
            D_RW(root)->rp = D_RO(root)->rp + 1;

            D_RW(root)->buf[D_RO(root)->wp] = D_RO(root)->buf[D_RO(root)->wp - 1] + D_RO(root)->buf[D_RO(root)->wp - 2];
            D_RW(root)->wp = D_RO(root)->wp + 1;
        } TX_END        
    }

    pmemobj_close(pop);
    return 0;
}