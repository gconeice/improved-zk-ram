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

uint64_t comm(BoolIO<NetIO> *ios[threads]) {
	uint64_t c = 0;
	for(int i = 0; i < threads; ++i)
		c += ios[i]->counter;
	return c;
}
uint64_t comm2(BoolIO<NetIO> *ios[threads]) {
	uint64_t c = 0;
	for(int i = 0; i < threads; ++i)
		c += ios[i]->io->counter;
	return c;
}

inline uint64_t calculate_hash(PRP &prp, uint64_t x) {
	block bk = makeBlock(0, x);
	prp.permute_block(&bk, 1);
	return LOW64(bk) % PR;
}

void test_circuit_zk(BoolIO<NetIO> *ios[threads], int party, uint64_t logN, int blockSize) {

    uint64_t RAM_N = 1;
    RAM_N <<= logN;

    int max_itr = 1<<(23/logN);
    uint64_t total_T = RAM_N * max_itr;
    
    vector<uint64_t> init_val;
    for (int i = 0; i < RAM_N; i++) init_val.push_back(i);

	uint64_t com1 = comm(ios);
	uint64_t com11 = comm2(ios);
	auto start = clock_start();

	setup_zk_arith<BoolIO<NetIO>>(ios, threads, party);

    IZKRAM zkram(RAM_N, total_T);
    zkram.Setup(init_val);

	auto time1 = time_from(start)/(max_itr*RAM_N);
	cout << "Setup: " << time1 << "us" << endl;
	auto start1 = clock_start();	
    
    for (int itr = 0; itr < max_itr; itr++)
        for (int i = 0; i < RAM_N; i++) {
            IntFp accid = IntFp(i, PUBLIC);
            IntFp wbval = IntFp(RAM_N*itr + i, PUBLIC);
			IntFp rw = IntFp(0, PUBLIC);
            zkram.Access(accid, wbval, rw);
        }

	auto time2 = time_from(start1)/(max_itr*RAM_N);
	cout << "Access: " << time2 << "us" << endl;

    zkram.Teardown_Batch(party, blockSize);

	finalize_zk_arith<BoolIO<NetIO>>();
	auto timeuse = time_from(start);	
	cout << "Teardown: " << timeuse/(max_itr*RAM_N)-time1-time2 << "us" << endl;	
	cout << logN << "\t" << RAM_N << "\t" << timeuse << " us\t" << party << " " << endl;
	cout << "time per access: " << timeuse/(max_itr*RAM_N) << "us" << endl;
	std::cout << std::endl;

	uint64_t com2 = comm(ios) - com1;
	uint64_t com22 = comm2(ios) - com11;
	std::cout << "communication (B): " << com2 << std::endl;
	std::cout << "communication (B): " << com22 << std::endl;
	cout << "comm per access: " << double(com2)/(max_itr*RAM_N) << "B" << endl;

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
		std::cout << "usage: bin/test_arith_zk_ram_block PARTY PORT logN blockSize" << std::endl;
		return -1;    
    }

	parse_party_and_port(argv, &party, &port);
	BoolIO<NetIO>* ios[threads];
	for(int i = 0; i < threads; ++i)
		ios[i] = new BoolIO<NetIO>(new NetIO(party == ALICE?nullptr:"127.0.0.1",port+i), party==ALICE);

	std::cout << std::endl << "------------ circuit zero-knowledge proof test ------------" << std::endl << std::endl;;

	int logN = 5;	
    int blockSize = 4;
	
	if (argc == 3) {
		logN = 5;	
        blockSize = 4;	
	} else {
		logN = atoi(argv[3]);		
        blockSize = atoi(argv[4]);
	}
	

	test_circuit_zk(ios, party, logN, blockSize);

	for(int i = 0; i < threads; ++i) {
		delete ios[i]->io;
		delete ios[i];
	}
	return 0;
}