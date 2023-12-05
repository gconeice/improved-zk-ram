# Improved Constant Overhead ZK RAM

This repository implements state-of-the-art arithmetic circuits for Zero Knowledge random access memory (RAM), read-only memory (ROM), and a set membership data structure.
See our paper for details.

System requirements and supported environments
=====
Our repository can be executed on a single machine to emulate ZK Prover P and ZK Verifier V using a localhost network.
However, our results were tested using two standalone machines: one for ZK Prover P, and another for ZK Verifier V.

We tested our code on two machines each having ≥ 32GiB
memory. Specifically, we used two Amazon Web Services (AWS)
EC2 `m5.2xlarge` machines.

We tested our code on a clean installation of Ubuntu 22.04.
We also tried our code on macOS. It works well but our
numbers were not generated over macOS.

Basis on EMP
=====
Our ZK RAM is based on QuickSilver's (https://eprint.iacr.org/2021/076) repository, which is part of the EMP Toolkit: https://github.com/emp-toolkit/emp-zk. In particular, we forked their repository and developed based on it. We also tweak some of EMP's libraries.
In our final open source version, we will further clarify all changes.

MIT license is included as part of EMP.

Installing EMP
=====
1. `wget https://raw.githubusercontent.com/emp-toolkit/emp-readme/master/scripts/install.py`
2. `python[3] install.py --deps --tool --ot --zk`
    1. By default it will build for Release. `-DCMAKE_BUILD_TYPE=[Release|Debug]` option is also available.
    2. No sudo? Change [`CMAKE_INSTALL_PREFIX`](https://cmake.org/cmake/help/v2.8.8/cmake.html#variable%3aCMAKE_INSTALL_PREFIX).

Build
=====
1. `mkdir build && cd build && cmake ../ && make`

Setup
=====
On the V's machine, please set the environment variable `IP` to the IP address of P's machine.

Browsing the code
=====
`/zk-ram` contains our core code.
`/test/arith` contains our test scaffolding (see next).
`/emp-zk` contains the EMP Toolkit's ZK library, including the implementation
of FKL+21 (see `/emp-zk/extensions/ram-zk`).

Test
=====
We include the following tests:
1. Our RAM:
   1. `/test/arith/zk_ram.cpp`: Our RAM without the high fan-in multiplication optimization.
   2. `/test/arith/zk_ram_block.cpp`: Our RAM with the high fan-in multiplication optimization.

2. Our ROM:
   1. `/test/arith/zk_rom.cpp`: Our ROM without the high fan-in multiplication optimization.
   2. `/test/arith/zk_rom_block.cpp`: Our ROM with the high fan-in multiplication optimization.

3. Our set data structure:
   1. `/test/arith/inset_zk_rom.cpp`: Our set data structure without the high fan-in multiplication optimization.
   2. `/test/arith/inset_zk_rom_block.cpp`: Our set data structure with the high fan-in multiplication optimization.

4. Our implementation of GOT+22:
   1. `/test/arith/GOT_nomulcheck.cpp`: Our implementation of GOT+22 without any optimization.
   2. `/test/arith/GOT.cpp`: Our implementation of GOT+22 without high-fan-in multiplication optimization.
   3. `/test/arith/GOT_block.cpp`: Our implementation of GOT+22 with all optimizations.

5. FKL+21 as baseline: The FKL+21 implementation is available here: https://github.com/emp-toolkit/emp-zk.

A toy test
=====
Please `cd` to the `build` folder.

On P's machine, execute: `./bin/test_arith_inset_zk_rom_block 1 44444 12 16 localhost`

On V's machine, execute: `./bin/test_arith_inset_zk_rom_block 2 44444 12 16 $IP`

If everything goes through, you should see the execution results on P and V.

Question
=====
Please email to Yibin Yang (yyang811@gatech.edu) and David Heath (daheath@illinois.edu).