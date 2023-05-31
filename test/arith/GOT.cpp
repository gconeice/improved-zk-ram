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
IntFp one;
IntFp minorone;
uint64_t delta;

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

uint64_t QuickExp(uint64_t base, uint64_t power) {
    uint64_t res = 1;
    while (power) {
        if (power & 1) res = mult_mod(res, base);
        power >>= 1;
        base = mult_mod(base, base);
    }
    return res;
}

vector<IntFp> queue_r;
vector<IntFp> queue_r_inv;
vector<IntFp> queue_b;

void Check_Queue_r(int &party) {
    block seed; 
    if (party == ALICE) {
        ZKFpExec::zk_exec->recv_data(&seed, sizeof(block));
    } else {
        PRG().random_block(&seed, 1);
        ZKFpExec::zk_exec->send_data(&seed, sizeof(block));
    }
    PRG prg(&seed); 

    uint64_t C0 = 0;
    uint64_t C1 = 0;
    uint64_t expected_value = 0;
    uint64_t delta_pow_2 = mult_mod(delta, delta);

    for (int i = 0; i < queue_r.size(); i++) {
        IntFp alpha = queue_r[i];
        IntFp one_alpha = queue_r_inv[i];
        // check alpha * one_alpha is 1
        uint64_t coeff;
        prg.random_data(&coeff, sizeof(uint64_t));
		coeff %= PR;   
        if (party == ALICE) {
            uint64_t add_term = mult_mod(LOW64(alpha.value), LOW64(one_alpha.value));
            C0 = add_mod(C0, mult_mod(add_term, coeff));
            add_term = PR - mult_mod(HIGH64(alpha.value), LOW64(one_alpha.value));
            C1 = add_mod(C1, mult_mod(add_term, coeff));
            add_term = PR - mult_mod(LOW64(alpha.value), HIGH64(one_alpha.value));
            C1 = add_mod(C1, mult_mod(add_term, coeff));
        } else {
            uint64_t add_term = mult_mod(LOW64(alpha.value), LOW64(one_alpha.value));
            expected_value = add_mod(expected_value, mult_mod(add_term, coeff));
            expected_value = add_mod(expected_value, PR - mult_mod(delta_pow_2, coeff));
        }
    }

    __uint128_t random_mask = ZKFpExec::zk_exec->get_one_role();
    if (party == ALICE) {
        C0 = add_mod(C0, LOW64(random_mask));
        C1 = add_mod(C1, PR - HIGH64(random_mask));
        ZKFpExec::zk_exec->send_data(&C0, sizeof(uint64_t));
        ZKFpExec::zk_exec->send_data(&C1, sizeof(uint64_t));
    } else {
        expected_value = add_mod(expected_value, LOW64(random_mask));
        ZKFpExec::zk_exec->recv_data(&C0, sizeof(uint64_t));
        ZKFpExec::zk_exec->recv_data(&C1, sizeof(uint64_t));
        uint64_t get_value = add_mod(C0, mult_mod(C1, delta));
        if (get_value != expected_value) error("Prover Cheat!!!");
    }        
}

void Check_Queue_b(int &party) {
    block seed; 
    if (party == ALICE) {
        ZKFpExec::zk_exec->recv_data(&seed, sizeof(block));
    } else {
        PRG().random_block(&seed, 1);
        ZKFpExec::zk_exec->send_data(&seed, sizeof(block));
    }
    PRG prg(&seed); 

    uint64_t C0 = 0;
    uint64_t C1 = 0;
    uint64_t expected_value = 0;

    for (int i = 0; i < queue_b.size(); i++) {
        IntFp alpha = queue_b[i];
        IntFp one_alpha = one + alpha.negate();
        // check alpha * (1-alpha) is 0
        uint64_t coeff;
        prg.random_data(&coeff, sizeof(uint64_t));
		coeff %= PR;   
        if (party == ALICE) {
            uint64_t add_term = mult_mod(LOW64(alpha.value), LOW64(one_alpha.value));
            C0 = add_mod(C0, mult_mod(add_term, coeff));
            add_term = PR - mult_mod(HIGH64(alpha.value), LOW64(one_alpha.value));
            C1 = add_mod(C1, mult_mod(add_term, coeff));
            add_term = PR - mult_mod(LOW64(alpha.value), HIGH64(one_alpha.value));
            C1 = add_mod(C1, mult_mod(add_term, coeff));
        } else {
            uint64_t add_term = mult_mod(LOW64(alpha.value), LOW64(one_alpha.value));
            expected_value = add_mod(expected_value, mult_mod(add_term, coeff));
        }
    }

    __uint128_t random_mask = ZKFpExec::zk_exec->get_one_role();
    if (party == ALICE) {
        C0 = add_mod(C0, LOW64(random_mask));
        C1 = add_mod(C1, PR - HIGH64(random_mask));
        ZKFpExec::zk_exec->send_data(&C0, sizeof(uint64_t));
        ZKFpExec::zk_exec->send_data(&C1, sizeof(uint64_t));
    } else {
        expected_value = add_mod(expected_value, LOW64(random_mask));
        ZKFpExec::zk_exec->recv_data(&C0, sizeof(uint64_t));
        ZKFpExec::zk_exec->recv_data(&C1, sizeof(uint64_t));
        uint64_t get_value = add_mod(C0, mult_mod(C1, delta));
        if (get_value != expected_value) error("Prover Cheat!!!");
    }    
}

IntFp EqCheck(IntFp &x, IntFp &y, int &party) {
    IntFp r;
    IntFp r_inv;
    if (party == ALICE) {
        uint64_t rval;
        uint64_t rval_inv = 1;
        if (HIGH64(x.value) != HIGH64(y.value)) rval_inv = add_mod(HIGH64(x.value), PR-HIGH64(y.value));
        rval = QuickExp(rval_inv, PR-2);
        r = IntFp(rval, ALICE); 
        r_inv = IntFp(rval_inv, ALICE);
    } else {
        r = IntFp(0, ALICE);
        r_inv = IntFp(0, ALICE);
    }
    // check r*r_inv == 1 later
    queue_r.push_back(r);
    queue_r_inv.push_back(r_inv);
    IntFp b = (x + y.negate()) * r;
    // check (1-b)*b == 0 later
    queue_b.push_back(b);
    return one + b.negate();
}

void BdCheck(int &party, uint64_t T, IntFp *x, uint64_t B1, uint64_t B2, int blockSize) {

    uint64_t S = B2 - B1 + 1 + T;
    vector<IntFp> sort_L;
    if (party == ALICE) {
        vector<uint64_t> sort_L_val;
        for (uint64_t i = B1; i <= B2; i++) sort_L_val.push_back(i);
        for (int i = 0; i < T; i++) sort_L_val.push_back(HIGH64(x[i].value));
        // sort and commit
        sort(sort_L_val.begin(), sort_L_val.end());
        assert(sort_L_val.size() == S);
        for (int i = 0; i < S; i++) sort_L.push_back(IntFp(sort_L_val[i], ALICE));
    } else {
        for (int i = 0; i < S; i++) sort_L.push_back(IntFp(0, ALICE));
    }

    //===========================================================
    // PermCheck
    // TODO: Optimize using epsilon
    uint64_t X; // evaluate at this value
    if (party == ALICE) {
        ZKFpExec::zk_exec->recv_data(&X, sizeof(uint64_t));
    } else {
        PRG tmpprg;
        tmpprg.random_data(&X, sizeof(uint64_t));
        X = X % PR;
        ZKFpExec::zk_exec->send_data(&X, sizeof(uint64_t));  
    }    

    uint64_t known_prod = 1; 
    for (uint64_t i = B1; i <= B2; i++) known_prod = mult_mod(known_prod, add_mod(i, X));
    IntFp prod_L = IntFp(known_prod, PUBLIC);
    IntFp prod_sort_L = IntFp(1, PUBLIC);
    for (int i = 0; i < T; i++) prod_L = prod_L * (x[i] + X);
    for (int i = 0; i < S; i++) prod_sort_L = prod_sort_L * (sort_L[i] + X);
    // check they are equal, namely, the same is zero
    IntFp final_zero = prod_L + prod_sort_L.negate();
	batch_reveal_check_zero(&final_zero, 1);

    //===========================================================

    block seed; 
    if (party == ALICE) {
        ZKFpExec::zk_exec->recv_data(&seed, sizeof(block));
    } else {
        PRG().random_block(&seed, 1);
        ZKFpExec::zk_exec->send_data(&seed, sizeof(block));
    }
    PRG prg(&seed); 

    uint64_t C0 = 0;
    uint64_t C1 = 0;
    uint64_t expected_value = 0;

    for (int i = 0; i < S-1; i++) {
        IntFp alpha = sort_L[i+1] + sort_L[i].negate();
        IntFp one_alpha = one + alpha.negate();
        // check alpha * (1-alpha) is 0
        uint64_t coeff;
        prg.random_data(&coeff, sizeof(uint64_t));
		coeff %= PR;   
        if (party == ALICE) {
            uint64_t add_term = mult_mod(LOW64(alpha.value), LOW64(one_alpha.value));
            C0 = add_mod(C0, mult_mod(add_term, coeff));
            add_term = PR - mult_mod(HIGH64(alpha.value), LOW64(one_alpha.value));
            C1 = add_mod(C1, mult_mod(add_term, coeff));
            add_term = PR - mult_mod(LOW64(alpha.value), HIGH64(one_alpha.value));
            C1 = add_mod(C1, mult_mod(add_term, coeff));
        } else {
            uint64_t add_term = mult_mod(LOW64(alpha.value), LOW64(one_alpha.value));
            expected_value = add_mod(expected_value, mult_mod(add_term, coeff));
        }
    }

    __uint128_t random_mask = ZKFpExec::zk_exec->get_one_role();
    if (party == ALICE) {
        C0 = add_mod(C0, LOW64(random_mask));
        C1 = add_mod(C1, PR - HIGH64(random_mask));
        ZKFpExec::zk_exec->send_data(&C0, sizeof(uint64_t));
        ZKFpExec::zk_exec->send_data(&C1, sizeof(uint64_t));
    } else {
        expected_value = add_mod(expected_value, LOW64(random_mask));
        ZKFpExec::zk_exec->recv_data(&C0, sizeof(uint64_t));
        ZKFpExec::zk_exec->recv_data(&C1, sizeof(uint64_t));
        uint64_t get_value = add_mod(C0, mult_mod(C1, delta));
        if (get_value != expected_value) error("Prover Cheat!!!");
    }

    IntFp check_first = sort_L[0] + (PR - B1);
    IntFp check_last = sort_L[S-1] + (PR - B2);
    batch_reveal_check_zero(&check_first, 1);
    batch_reveal_check_zero(&check_last, 1);
}

struct GOTTuple {
    IntFp l;
    IntFp t;
    IntFp op;
    IntFp d;
};

struct GOTSortTuple {
    uint64_t l;
    uint64_t t;
    uint64_t op;
    uint64_t d;
    bool operator<(const GOTSortTuple& a) const
    {
        return (l < a.l) | ( (l == a.l) & (t < a.t) );
    }    
};

void test_circuit_zk(BoolIO<NetIO> *ios[threads], int party, uint64_t logN, int blockSize) {

    uint64_t RAM_N = 1;
    RAM_N <<= logN;
	int max_itr = 1<<(23-logN);

    uint64_t com1 = comm(ios);
	uint64_t com11 = comm2(ios);
    setup_zk_arith<BoolIO<NetIO>>(ios, threads, party);
    one = IntFp(1, PUBLIC);
    minorone = IntFp(PR-1, PUBLIC);
    delta = ZKFpExec::zk_exec->get_delta();
    
    vector<GOTTuple> L; vector<GOTSortTuple> pre_sort;
    for (int i = 0; i < RAM_N; i++) {
        GOTTuple tmp;
        tmp.l = IntFp(i, PUBLIC); tmp.t = IntFp(i+1, PUBLIC);
        tmp.op = IntFp(1, PUBLIC); tmp.d = IntFp(i, PUBLIC);
        L.push_back(tmp);
        GOTSortTuple tmp1;
        tmp1.l = i; tmp1.t = i+1;
        tmp1.op = 1; tmp1.d = i;
        pre_sort.push_back(tmp1);
    }
    for (int i = 0; i < max_itr; i++) 
        for (int j = 0; j < RAM_N; j++) {
            GOTTuple tmp;
            tmp.l = IntFp(j, PUBLIC); tmp.t = IntFp(RAM_N + i*RAM_N + j + 1, PUBLIC);
            tmp.op = IntFp(0, PUBLIC); tmp.d = IntFp(j, PUBLIC);
            L.push_back(tmp);
            GOTSortTuple tmp1;
            tmp1.l = j; tmp1.t = RAM_N + i*RAM_N + j + 1;
            tmp1.op = 0; tmp1.d = j;
            pre_sort.push_back(tmp1);
        }
    
	auto start = clock_start();   

    vector<GOTTuple> sort_L;
    if (party == ALICE) {      
        sort(pre_sort.begin(), pre_sort.end());  
        for (int i = 0; i < pre_sort.size(); i++) {
            GOTTuple tmp;
            tmp.l = IntFp(pre_sort[i].l, ALICE);
            tmp.t = IntFp(pre_sort[i].t, ALICE);
            tmp.op = IntFp(pre_sort[i].op, ALICE);
            tmp.d = IntFp(pre_sort[i].d, ALICE);
            sort_L.push_back(tmp);
        }
    } else {
        for (int i = 0; i < pre_sort.size(); i++) {
            GOTTuple tmp;
            tmp.l = IntFp(0, ALICE);
            tmp.t = IntFp(0, ALICE);
            tmp.op = IntFp(0, ALICE);
            tmp.d = IntFp(0, ALICE);
            sort_L.push_back(tmp);
        }
    }

    //===========================================================
    // 1st Permcheck 
    // TODO: optimization for epsilon

    uint64_t A0, A1, A2, A3; // to compress the tuple into one IntFp
    uint64_t X; // evaluate at this value
    if (party == ALICE) {
        ZKFpExec::zk_exec->recv_data(&A0, sizeof(uint64_t));
        ZKFpExec::zk_exec->recv_data(&A1, sizeof(uint64_t));
        ZKFpExec::zk_exec->recv_data(&A2, sizeof(uint64_t));
        ZKFpExec::zk_exec->recv_data(&A3, sizeof(uint64_t));
        ZKFpExec::zk_exec->recv_data(&X, sizeof(uint64_t));
    } else {
        PRG tmpprg;
		tmpprg.random_data(&A0, sizeof(uint64_t));
		A0 = A0 % PR;
		tmpprg.random_data(&A1, sizeof(uint64_t));
		A1 = A1 % PR;
		tmpprg.random_data(&A2, sizeof(uint64_t));
		A2 = A2 % PR;
		tmpprg.random_data(&A3, sizeof(uint64_t));
		A3 = A3 % PR;
        tmpprg.random_data(&X, sizeof(uint64_t));
        X = X % PR;
		ZKFpExec::zk_exec->send_data(&A0, sizeof(uint64_t));  
        ZKFpExec::zk_exec->send_data(&A1, sizeof(uint64_t));  
        ZKFpExec::zk_exec->send_data(&A2, sizeof(uint64_t));  
        ZKFpExec::zk_exec->send_data(&A3, sizeof(uint64_t));  
        ZKFpExec::zk_exec->send_data(&X, sizeof(uint64_t));  
    }
	    
    IntFp prod_L = IntFp(1, PUBLIC);
    IntFp prod_sort_L = IntFp(1, PUBLIC);
    for (int i = 0; i < L.size(); i++) {
        prod_L = prod_L * (L[i].l * A0 + L[i].t * A1 + L[i].op * A2 + L[i].d * A3 + X);
        prod_sort_L = prod_sort_L * (sort_L[i].l * A0 + sort_L[i].t * A1 + sort_L[i].op * A2 + sort_L[i].d * A3 + X);
    }
    // check they are equal, namely, the same is zero
    IntFp final_zero = prod_L + prod_sort_L.negate();
	batch_reveal_check_zero(&final_zero, 1);

    //===========================================================

    IntFp *check_zero = new IntFp[2*(sort_L.size() - 1)];
    IntFp *tau = new IntFp[sort_L.size() - 1];
    int check_zero_cnt = 0;
    for (int i = 0; i < sort_L.size()-1; i++) {
        // Eqcheck(l'_i == L'_i+1)
        IntFp alpha = EqCheck(sort_L[i].l, sort_L[i+1].l, party);
        IntFp lambda = sort_L[i+1].l + sort_L[i].l.negate();
        tau[i] = alpha * (sort_L[i+1].t + sort_L[i].t.negate()) + (one + alpha.negate());
        check_zero[check_zero_cnt++] = alpha + lambda + minorone;
        IntFp beta = EqCheck(sort_L[i].d, sort_L[i+1].d, party);
        check_zero[check_zero_cnt++] = alpha * (one + beta.negate()) * (one + sort_L[i+1].op.negate());
    }
    batch_reveal_check_zero(check_zero, check_zero_cnt);
    Check_Queue_b(party);
    Check_Queue_r(party);

    //===========================================================
    //BdCheck
    BdCheck(party, sort_L.size() - 1, tau, 1, sort_L.size() - 1, blockSize);

    //===========================================================

    IntFp last_l_check = sort_L[sort_L.size()-1].l + IntFp(RAM_N-1, PUBLIC).negate();
    batch_reveal_check_zero(&last_l_check, 1);

	finalize_zk_arith<BoolIO<NetIO>>();
	auto timeuse = time_from(start);	
	//cout << "Teardown: " << timeuse/(max_itr*ROM_N)-time1-time2 << "us" << endl;
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
    if(argc < 4) {
		std::cout << "usage: bin/test_arith_zk_rom_block PARTY PORT logN blockSize" << std::endl;
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