#ifndef IMPROVE_ZK_RAM
#define IMPROVE_ZK_RAM

#include "emp-zk/emp-zk.h"
#include <iostream>
#include "emp-tool/emp-tool.h"
#include "inset_rom.h"
#include <vector>
#if defined(__linux__)
	#include <sys/time.h>
	#include <sys/resource.h>
#elif defined(__APPLE__)
	#include <unistd.h>
	#include <sys/resource.h>
	#include <mach/mach.h>
#endif

using namespace std;
using namespace emp;

struct RAMTuple{
    IntFp idx; // the address or index
    IntFp timestamp; // which time it is
    IntFp val; // the value inside
};

class IZKRAM {
public:
    uint64_t N; // size of the ram
    uint64_t T; // number of access
    std::vector<RAMTuple> read_list;
    std::vector<RAMTuple> write_list;
    // inset ROM for bound_check
    InsetZKROM *bound_check_rom;
    // Alice/P uses latest_pos to find the latest place of an index
    std::vector<uint64_t> latest_pos; 
    IntFp one;

    IZKRAM(uint64_t N, uint64_t total_T) {
        this->N = N;
        this->T = 0;
        bound_check_rom = new InsetZKROM(total_T);        
        one = IntFp(1, PUBLIC);
    }

    void Setup(std::vector<uint64_t>& init_val) {        
        RAMTuple tmp;
        for (int i = 0; i < N; i++) {
            tmp.idx = IntFp(i, PUBLIC);
            tmp.timestamp = IntFp(0, PUBLIC);
            tmp.val = IntFp(init_val[i], ALICE);
            write_list.push_back(tmp);
            latest_pos.push_back(i);
        }
        // Setup bound_check inset_rom
        bound_check_rom->Setup();
    }

    IntFp Access(IntFp& id, IntFp& val) {
        uint64_t addr = HIGH64(id.value);
        RAMTuple tmp_w, tmp_r;
        tmp_r.idx = tmp_w.idx = id;
        uint64_t pos = latest_pos[addr];
        // two inputs
        IntFp old_timestamp = IntFp(HIGH64(write_list[pos].timestamp.value), ALICE);
        IntFp old_val = IntFp(HIGH64(write_list[pos].val.value), ALICE);
        tmp_r.timestamp = old_timestamp;
        tmp_r.val = old_val;
        tmp_w.timestamp = IntFp(++T, PUBLIC);
        tmp_w.val = val;
        read_list.push_back(tmp_r);
        write_list.push_back(tmp_w);
        // boundcheck access
        IntFp diff = tmp_w.timestamp + tmp_r.timestamp.negate();
        bound_check_rom->Access(diff);
        // Update the lastest pos and increase the T's counter
        latest_pos[addr] = N + T - 1;
        return old_val;
    }

    void Teardown_Basic(int &party) {
        RAMTuple tmp;
        for (int i = 0; i < N; i++) {
            tmp.idx = IntFp(i, PUBLIC);
            uint64_t pos = latest_pos[i]; // the last pos
            tmp.timestamp = IntFp(HIGH64(write_list[pos].timestamp.value), ALICE);            
            tmp.val = IntFp(HIGH64(write_list[pos].val.value), ALICE);
            read_list.push_back(tmp);
            // boundcheck access
            // bound_check_rom->Access(IntFp(T+1, PUBLIC) - tmp.timestamp);
            // Dicuss this with Dave
        }

        // bound check
        bound_check_rom->Teardown_Basic(party);

        uint64_t A0, A1, A2; // to compress the tuple into one IntFp
        uint64_t X; // evaluate at this value
        if (party == ALICE) {
            ZKFpExec::zk_exec->recv_data(&A0, sizeof(uint64_t));
            ZKFpExec::zk_exec->recv_data(&A1, sizeof(uint64_t));
            ZKFpExec::zk_exec->recv_data(&A2, sizeof(uint64_t));
            ZKFpExec::zk_exec->recv_data(&X, sizeof(uint64_t));
        } else {
            PRG tmpprg;
		    tmpprg.random_data(&A0, sizeof(uint64_t));
		    A0 = A0 % PR;
		    tmpprg.random_data(&A1, sizeof(uint64_t));
		    A1 = A1 % PR;
		    tmpprg.random_data(&A2, sizeof(uint64_t));
		    A2 = A2 % PR;
            tmpprg.random_data(&X, sizeof(uint64_t));
            X = X % PR;
		    ZKFpExec::zk_exec->send_data(&A0, sizeof(uint64_t));  
            ZKFpExec::zk_exec->send_data(&A1, sizeof(uint64_t));  
            ZKFpExec::zk_exec->send_data(&A2, sizeof(uint64_t));  
            ZKFpExec::zk_exec->send_data(&X, sizeof(uint64_t));  
        }
	    
        IntFp prod_read = IntFp(1, PUBLIC);
        IntFp prod_write = IntFp(1, PUBLIC);
        for (int i = 0; i < N+T; i++) {
            prod_read = prod_read * (read_list[i].idx * A0 + read_list[i].timestamp * A1 + read_list[i].val * A2 + X);
            prod_write = prod_write * (write_list[i].idx * A0 + write_list[i].timestamp * A1 + write_list[i].val * A2 + X);
        }
        // check they are equal, namely, the same is zero
        IntFp final_zero = prod_read + prod_write.negate();
	    batch_reveal_check_zero(&final_zero, 1);
        
    }

};

#endif