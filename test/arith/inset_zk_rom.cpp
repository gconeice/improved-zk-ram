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

    uint64_t ROM_N = 1;
    ROM_N <<= logN;
    
	auto start = clock_start();

	setup_zk_arith<BoolIO<NetIO>>(ios, threads, party);

    InsetZKROM zkrom(ROM_N);
    zkrom.Setup();
    
    for (int itr = 0; itr < 20; itr++)
        for (int i = 1; i <= ROM_N; i++) {
            IntFp accid = IntFp(i, PUBLIC);
            zkrom.Access(accid);
        }

	// cheating testing
	// IntFp accid = IntFp(0, PUBLIC);
	// zkrom.Access(accid);

    zkrom.Teardown_Basic(party);

	finalize_zk_arith<BoolIO<NetIO>>();
	auto timeuse = time_from(start);	
	cout << logN << "\t" << ROM_N << "\t" << timeuse << " us\t" << party << " " << endl;
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
		std::cout << "usage: bin/test_arith_inset_zk_rom PARTY PORT logN" << std::endl;
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