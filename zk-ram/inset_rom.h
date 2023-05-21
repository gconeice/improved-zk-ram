#ifndef ZK_INSET_ROM
#define ZK_INSET_ROM

#include "emp-zk/emp-zk.h"
#include <iostream>
#include "emp-tool/emp-tool.h"
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

struct InsetROMTuple{
    IntFp idx; // the address or index
    IntFp version; // which version it is
};

class InsetZKROM {
public:
    uint64_t N; // size of the rom
    uint64_t T; // number of access
    std::vector<InsetROMTuple> read_list;
    std::vector<InsetROMTuple> write_list;
    // Alice/P uses latest_pos to find the latest place of an index
    std::vector<uint64_t> latest_pos; 
    IntFp one;

    InsetZKROM(uint64_t N) {
        this->N = N;
        this->T = 0;
        one = IntFp(1, PUBLIC);
        latest_pos.push_back(0);
    }

    void Setup() {
        InsetROMTuple tmp;
        for (int i = 1; i <= N; i++) {
            tmp.idx = IntFp(i, PUBLIC);
            tmp.version = IntFp(0, PUBLIC);
            write_list.push_back(tmp);
            latest_pos.push_back(i-1);
        }
    }
    
    
    void Access(IntFp& id) {
        uint64_t addr = HIGH64(id.value);
        InsetROMTuple tmp_w, tmp_r;
        tmp_r.idx = tmp_w.idx = id;
        uint64_t pos = latest_pos[addr];
        // two inputs
        IntFp old_version = IntFp(HIGH64(write_list[pos].version.value), ALICE);
        tmp_r.version = old_version;
        tmp_w.version = tmp_r.version + one;
        read_list.push_back(tmp_r);
        write_list.push_back(tmp_w);
        // Update the lastest pos and increase the T's counter
        latest_pos[addr] = N + T++;
    }

    void Teardown_Basic(int &party) {
        InsetROMTuple tmp;
        for (int i = 1; i <= N; i++) {
            tmp.idx = IntFp(i, PUBLIC);
            uint64_t pos = latest_pos[i]; // the last pos
            tmp.version = IntFp(HIGH64(write_list[pos].version.value), ALICE);            
            read_list.push_back(tmp);
        }

        uint64_t A0, A1; // to compress the tuple into one IntFp
        uint64_t X; // evaluate at this value
        if (party == ALICE) {
            ZKFpExec::zk_exec->recv_data(&A0, sizeof(uint64_t));
            ZKFpExec::zk_exec->recv_data(&A1, sizeof(uint64_t));
            ZKFpExec::zk_exec->recv_data(&X, sizeof(uint64_t));
        } else {
            PRG tmpprg;
		    tmpprg.random_data(&A0, sizeof(uint64_t));
		    A0 = A0 % PR;
		    tmpprg.random_data(&A1, sizeof(uint64_t));
		    A1 = A1 % PR;
            tmpprg.random_data(&X, sizeof(uint64_t));
            X = X % PR;
		    ZKFpExec::zk_exec->send_data(&A0, sizeof(uint64_t));  
            ZKFpExec::zk_exec->send_data(&A1, sizeof(uint64_t));  
            ZKFpExec::zk_exec->send_data(&X, sizeof(uint64_t));  
        }
	    
        IntFp prod_read = IntFp(1, PUBLIC);
        IntFp prod_write = IntFp(1, PUBLIC);
        for (int i = 0; i < N+T; i++) {
            prod_read = prod_read * (read_list[i].idx * A0 + read_list[i].version * A1 + X);
            prod_write = prod_write * (write_list[i].idx * A0 + write_list[i].version * A1 + X);
        }
        // check they are equal, namely, the same is zero
        IntFp final_zero = prod_read + prod_write.negate();
	    batch_reveal_check_zero(&final_zero, 1);
        
    }
    

};

#endif