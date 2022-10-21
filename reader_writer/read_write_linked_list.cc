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
    pobj::p<int> prev_data;
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
    std::cout << "**** Hi! This is a sample implementation of reader_writer program using linked list approach on persistent Memory ****" << std::endl;
    std::cout << "**************************************** This program will print Lucas series ****************************************" << std::endl; 
    pobj::pool<root> pop;
    pobj::persistent_ptr<root> proot;

    try {
		if (file_exists(path) != 0) {
            std::cout << "\t ----- File Not exist ----" << std::endl;
	        std::cout << "\t ----- Creating pool file ----" << std::endl;

	        pop = pobj::pool<root>::create(path, LAYOUT, 100000000, CREATE_MODE_RW);
            proot = pop.root();
            pobj::transaction::run(pop, [&] {
                std::cout << "\t ---- Initialisation ----" << std::endl;
		        std::cout << "\t\t\t Initial element of series: 2 1 " << std::endl;
                auto n = pobj::make_persistent<Node>();
                n->prev_data = 2;
		        n->data = 1;
                n->next = nullptr;

                if (proot->wp == nullptr && proot->rp == nullptr) {
                    proot->wp = proot->rp = n;
                }

		        std::cout << "\t ---- Read Pointer initialised successfully ----" << std::endl;
                auto n2 = pobj::make_persistent<Node>();
		        n2->prev_data = 1;
                n2->data = 3;
                n2->next = nullptr;
                
                proot->wp->next = n2;
                proot->wp = n2;
		        std::cout << "\t ---- Write Pointer initialised successfully ----" << std::endl;
		        std::cout << "\t\t\t Write: " << n2->data << std::endl;
            });
		} else {
            //File exists
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
	    	n->prev_data = proot->wp->data;
                n->data = proot->wp->data + proot->wp->prev_data;
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
	    std::cout << "\t\t\t Write: " << proot->wp->data << std::endl;
        });

        // read
        pobj::transaction::run(pop, [&] {
            if (proot->rp == nullptr)
                pobj::transaction::abort(EINVAL);

            uint64_t ret = proot->rp->data;
            auto n = proot->rp->next;

            pobj::delete_persistent<Node>(proot->rp);
                    std::cout << "\t\t\t\t\t Read: " << ret << std::endl;
            proot->rp = n;

            if (proot->rp == nullptr)
                proot->wp = nullptr;
        });
    }
    return 0;
}
