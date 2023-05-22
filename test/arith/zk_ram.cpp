#include "emp-zk/emp-zk.h"
#include <iostream>
#include "emp-tool/emp-tool.h"
#include "zk-ram/zk-ram.h"
#if defined(__linux__)
	#include <sys/time.h>
	#include <sys/resource.h>
#elif defined(__APPLE__)
	#include <unistd.h>
	#include <sys/resource.h>
	#include <mach/mach.h>
#endif

using namespace emp;
using namespace std;

int port, party;
const int threads = 1;


inline uint64_t calculate_hash(PRP &prp, uint64_t x) {
	block bk = makeBlock(0, x);
	prp.permute_block(&bk, 1);
	return LOW64(bk) % PR;
}

void test_circuit_zk(BoolIO<NetIO> *ios[threads], int party, uint64_t logN) {

    uint64_t RAM_N = 1;
    RAM_N <<= logN;

    int max_itr = 20;
    uint64_t total_T = RAM_N * max_itr;
    
    vector<uint64_t> init_val;
    for (int i = 0; i < RAM_N; i++) init_val.push_back(i);

	auto start = clock_start();

	setup_zk_arith<BoolIO<NetIO>>(ios, threads, party);

    IZKRAM zkram(RAM_N, total_T);
    zkram.Setup(init_val);
    
    for (int itr = 0; itr < max_itr; itr++)
        for (int i = 0; i < RAM_N; i++) {
            IntFp accid = IntFp(i, PUBLIC);
            IntFp wbval = IntFp(RAM_N*itr + i, PUBLIC);
            zkram.Access(accid, wbval);
        }

    zkram.Teardown_Basic(party);

	finalize_zk_arith<BoolIO<NetIO>>();
	auto timeuse = time_from(start);	
	cout << logN << "\t" << RAM_N << "\t" << timeuse << " us\t" << party << " " << endl;
	std::cout << std::endl;


#if defined(__linux__)
	struct rusage rusage;
	if (!getrusage(RUSAGE_SELF, &rusage))
		std::cout << "[Linux]Peak resident set size: " << (size_t)rusage.ru_maxrss << std::endl;
	else std::cout << "[Linux]Query RSS failed" << std::endl;
#elif defined(__APPLE__)
	struct mach_task_basic_info info;
	mach_msg_type_number_t count = MACH_TASK_BASIC_INFO_COUNT;
	if (task_info(mach_task_self(), MACH_TASK_BASIC_INFO, (task_info_t)&info, &count) == KERN_SUCCESS)
		std::cout << "[Mac]Peak resident set size: " << (size_t)info.resident_size_max << std::endl;
	else std::cout << "[Mac]Query RSS failed" << std::endl;
#endif
}

int main(int argc, char** argv) {
    if(argc < 3) {
		std::cout << "usage: bin/test_arith_zk_ram PARTY PORT logN" << std::endl;
		return -1;    
    }

	parse_party_and_port(argv, &party, &port);
	BoolIO<NetIO>* ios[threads];
	for(int i = 0; i < threads; ++i)
		ios[i] = new BoolIO<NetIO>(new NetIO(party == ALICE?nullptr:"127.0.0.1",port+i), party==ALICE);

	std::cout << std::endl << "------------ circuit zero-knowledge proof test ------------" << std::endl << std::endl;;

	int logN = 5;	
	
	if (argc == 3) {
		logN = 5;		
	} else {
		logN = atoi(argv[3]);		
	}
	

	test_circuit_zk(ios, party, logN);

	for(int i = 0; i < threads; ++i) {
		delete ios[i]->io;
		delete ios[i];
	}
	return 0;
}