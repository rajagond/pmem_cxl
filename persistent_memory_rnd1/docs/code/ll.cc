/*
* Resources:  Programming Persistent Memory
                - A Comprehensive Guide for Developers by Steve Scargall
* Full code can be found [here]
(https://github.com/rajagond/pmem_cxl/blob/main/reader_writer/read_write_linked_list.cc)
* This program will print the Lucas series
*/
#define CREATE_MODE_RW (S_IWUSR | S_IRUSR)
inline int file_exists (const std::string& name) {
    return access( name.c_str(), F_OK );
}
using namespace std;                  namespace pobj = pmem::obj;
#define LAYOUT "demo"
struct Node {
    pobj::p<int> prev_data;           pobj::p<int> data;
    pobj::persistent_ptr<Node> next;
};
class root {
    public:
        pobj::persistent_ptr<Node> wp = nullptr;  
        pobj::persistent_ptr<Node> rp = nullptr;
};
int main() {
    string path = "/mnt/pmem/po/poolfile";
    pobj::pool<root> pop;    pobj::persistent_ptr<root> proot;
    try {
		if (file_exists(path) != 0) {//File not exist
	        pop = pobj::pool<root>::create(path, LAYOUT, 100000000, 
                      CREATE_MODE_RW);
                proot = pop.root();
                /*
                * Initialisation using transactions
                */
		} else {//File exists
			pop = pobj::pool<root>::open(path, LAYOUT); 
                    proot = pop.root();
		}
	} catch (const pmem::pool_error &e) {
		std::cerr << "Exception: " << e.what() << std::endl; return 1;
	} catch (const pmem::transaction_error &e) {
		std::cerr << "Exception: " << e.what() << std::endl; return 1;
    }
    while(1){//run continuously
        sleep(1); //sleeping for one second
        pobj::transaction::run(pop, [&] { //write
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
        pobj::transaction::run(pop, [&] {// read
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