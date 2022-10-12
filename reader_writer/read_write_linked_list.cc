/*
resources:  Programming Persistent Memory
                - A Comprehensive Guide for Developers
            Steve Scargall
*/

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <libpmemobj++/make_persistent.hpp>
#include <libpmemobj++/p.hpp>
#include <libpmemobj++/persistent_ptr.hpp>
#include <libpmemobj++/pool.hpp>
#include <libpmemobj++/transaction.hpp>
#include <stdexcept>
#include <string>
#include <sys/stat.h>
#include <unistd.h>
#include <cstdint>

#define CREATE_MODE_RW (S_IWUSR | S_IRUSR)

/*
 * file_exists -- checks if file exists
 */
// static inline int
// file_exists(char const *file)
// {
// 	return access(file, F_OK);
// }
inline int file_exists (const std::string& name) {
    return access( name.c_str(), F_OK );
}

using namespace std;
namespace pobj = pmem::obj;

#define LAYOUT "demo"

struct Node {
    pobj::p<int> data;
    pobj::persistent_ptr<Node> next;
};

class root {
    public:
        pobj::persistent_ptr<Node> wp = nullptr;
	    pobj::persistent_ptr<Node> rp = nullptr;
};

int main() {
    string path = "/mnt/pmem/po/poolfile";
    pobj::pool<root> pop;
    pobj::persistent_ptr<root> proot;
    try {
		if (file_exists(path) != 0) {
            std::cout << "File Not exist" << std::endl;
			pop = pobj::pool<root>::create(path, LAYOUT, 100000000, CREATE_MODE_RW);
            proot = pop.root();
            pobj::transaction::run(pop, [&] {
                std::cout << "Initialisation" << std::endl;
                auto n = pobj::make_persistent<Node>();
                n->data = 1;
                n->next = nullptr;

                if (proot->wp == nullptr && proot->rp == nullptr) {
                    proot->wp = proot->rp = n;
                }

                auto n2 = pobj::make_persistent<Node>();
                n2->data = 2;
                n2->next = nullptr;
                
                proot->wp->next = n2;
                proot->wp = n2;
            });
		} else {
			pop = pobj::pool<root>::open(path, LAYOUT);
            proot = pop.root();
		}
		
	} catch (const pmem::pool_error &e) {
		std::cerr << "Exception: " << e.what() << std::endl;
		return 1;
	} catch (const pmem::transaction_error &e) {
		std::cerr << "Exception: " << e.what() << std::endl;
		return 1;
	}


    while(1){ 
        //sleeping for one second
        sleep(1);

        //write
        pobj::transaction::run(pop, [&] {
            auto n = pobj::make_persistent<Node>();

            if( proot->wp != nullptr){
                n->data = proot->wp->data + 1;
            }
            else{
                n->data = 1;
            }
            
            n->next = nullptr;

            if (proot->rp == nullptr && proot->wp == nullptr) {
                proot->rp = proot->wp = n;
            } else {
                proot->wp->next = n;
                proot->wp = n;
            }
        });

        // read
		pobj::transaction::run(pop, [&] {
			if (proot->rp == nullptr)
				pobj::transaction::abort(EINVAL);

			uint64_t ret = proot->rp->data;
			auto n = proot->rp->next;

			pobj::delete_persistent<Node>(proot->rp);
            std::cout << "Read: " << ret << std::endl;
			proot->rp = n;

			if (proot->rp == nullptr)
				proot->wp = nullptr;
		});
    }
    return 0;
}
//     PMEMobjpool *pop= pmemobj_open("/mnt/pmem/po/poolfile", "layout");

//     if (pop == NULL) {
//         pop = pmemobj_create("/mnt/pmem/po/poolfile", "layout", PMEMOBJ_MIN_POOL, 0777); //(100000000)
//         printf("create done\n"); 
//         //fibonacci
//         TX_BEGIN(pop) {
//             TOID(struct my_root) root = POBJ_ROOT(pop, struct my_root);

//             // adding full root to the transaction
//             TX_ADD(root);

//             D_RW(root)->rp = 0;
//             D_RW(root)->buf[0] = 0;
//             D_RW(root)->wp = 1;
//         } TX_END
//         printf("1 transaction done\n");
//         TX_BEGIN(pop) {
//             TOID(struct my_root) root = POBJ_ROOT(pop, struct my_root);

//             // adding full root to the transaction
//             TX_ADD(root);

//             D_RW(root)->buf[D_RO(root)->wp] = 1;
//             D_RW(root)->wp = D_RO(root)->wp + 1;
//         } TX_END
//     }

//     PMEMoid root = pmemobj_root(pop, sizeof(struct my_root));
//     struct my_root *rootp = pmemobj_direct(root);

//     int i = rootp->wp;

 
//     for (; i < 50; i++)
//     {
        
//         /* Add the previous 2 numbers in the series and store it */
//         TX_BEGIN(pop) {
//             TOID(struct my_root) root = POBJ_ROOT(pop, struct my_root);

//             // adding full root to the transaction
//             TX_ADD(root);
//             printf("%d ", D_RO(root)->buf[D_RO(root)->rp]);
//             fflush(stdout);
//             D_RW(root)->rp = D_RO(root)->rp + 1;

//             D_RW(root)->buf[D_RO(root)->wp] = D_RO(root)->buf[D_RO(root)->wp - 1] + D_RO(root)->buf[D_RO(root)->wp - 2];
//             D_RW(root)->wp = D_RO(root)->wp + 1;
//         } TX_END        
//     }

//     pmemobj_close(pop);
//     return 0;
// }

// struct my_data {
//     my_data(int a, int b): a(a), b(b) {}
//     int a;
//     int b;
//     };
    
// struct root {
//     pmem::obj::persistent_ptr<my_data> mdata;
//     };

// int main(int argc, char *argv[]) {
//     auto pop = pmem::obj::pool<root>::open("/daxfs/file", "tx");
    
//     auto r = pop.root();
    
//     pmem::obj::transaction::run(pop, [&]() {
//           r->mdata = pmem::obj::make_persistent<my_data>(1, 2);
//     });

//     pmem::obj::transaction::run(pop, [&]() {
//         pmem::obj::delete_persistent<my_data>(r->mdata);
//     });
        
//     pmem::obj::make_persistent_atomic<my_data>(pop, r->mdata, 2, 3);

//     return 0;
    
// }