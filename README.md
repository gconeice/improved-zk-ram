# Improved Constant Overhead ZK RAM

Base
=====
We acknowledge that our protocols based on QuickSilver repo available at: https://github.com/emp-toolkit/emp-zk.
In particular, we folk the repo and develop based on it.
We also tweat some emp libraries.
We will further clarify and obey the license in their repo in the final open source version.

Installation EMP libraries
=====
1. `wget https://raw.githubusercontent.com/emp-toolkit/emp-readme/master/scripts/install.py`
2. `python[3] install.py --deps --tool --ot --zk`
    1. By default it will build for Release. `-DCMAKE_BUILD_TYPE=[Release|Debug]` option is also available.
    2. No sudo? Change [`CMAKE_INSTALL_PREFIX`](https://cmake.org/cmake/help/v2.8.8/cmake.html#variable%3aCMAKE_INSTALL_PREFIX).

Build
=====
1. `mkdir build && cd build && cmake ../ && make`

Test
=====
We have the following tests:
1. Our RAM:
   1. arith_zk_ram: Our RAM without optimization.
   2. arith_zk_ram_block: Our RAM with optimization.

2. Our ROM:
   1. arith_zk_rom: Our ROM without optimization.
   2. arith_zk_rom_block: Our ROM with optimization.

3. Our set data structure:
   1. arith_inset_zk_rom: Our set data structure without optimization.
   2. arith_inset_zk_rom_block: Our set data structure with optimization.

4. Our implementation of GOT+22:
   1. arith_GOT_nomulcheck: Our implementation of GOT+22 without any optimization.
   2. arith_GOT: Our implementation of GOT+22 without high-fan-in multiplication optimization.
   3. arith_GOT_block: Our implementation of GOT+22 with all optimizations.

Core Header Files
=====
zk-ram folder includes the core codes.
