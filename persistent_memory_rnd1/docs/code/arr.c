/*
* Resources:  Programming Persistent Memory
                - A Comprehensive Guide for Developers by Steve Scargall
* Full code can be found [here]
(https://github.com/rajagond/pmem_cxl/blob/main/reader_writer/read_write.c)
*/
#define MAX_BUF_LEN 50
struct my_root {
    int rp; //for read
    int wp; //for write
    int buf[MAX_BUF_LEN]; //buffer
};
int main(int argc, char *argv[]) {
    char path_obj[50]; 
    scanf("%s",path_obj);//reading string
    PMEMobjpool *pop= pmemobj_open(path_obj, POBJ_LAYOUT_NAME(rweg));
    if (pop == NULL) {
        pop = pmemobj_create(path_obj, POBJ_LAYOUT_NAME(rweg), (100000000), 0666);
       /*
        * Initialisation of Fibonacci Series 
        * I have used transactions to read or write an element.
      */
    }
    PMEMoid root = pmemobj_root(pop, sizeof(struct my_root));
    struct my_root *rootp = pmemobj_direct(root);
    int i = rootp->wp;
    for (; ; i++)
    {
        sleep(1);
        TX_BEGIN(pop) {/* Add the previous 2 numbers in the series and store it */
            TOID(struct my_root) root = POBJ_ROOT(pop, struct my_root);
            TX_ADD(root); // adding full root to the transaction
            printf("%d ", D_RO(root)->buf[D_RO(root)->rp]);
            fflush(stdout);
            D_RW(root)->rp = D_RO(root)->rp + 1;
            int curr = D_RO(root)->buf[D_RO(root)->wp - 1];
            int prev = D_RO(root)->buf[D_RO(root)->wp - 2];
            D_RW(root)->buf[D_RO(root)->wp] = curr + prev; 
            D_RW(root)->wp = D_RO(root)->wp + 1;
        } TX_END        
    }
    pmemobj_close(pop);
    return 0;
}