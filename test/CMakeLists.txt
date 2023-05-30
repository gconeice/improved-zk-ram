#Testing macro
macro (add_test_executable_with_lib folder _name)
	add_executable(test_${folder}_${_name} "${folder}/${_name}.cpp")
	target_link_libraries(test_${folder}_${_name} ${EMP-OT_LIBRARIES} emp-zk)
endmacro()

macro (add_test_case folder _name)
	add_test_executable_with_lib(${folder} ${_name})
  	add_test(NAME ${folder}_${_name} COMMAND "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/test_${folder}_${_name}" WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/")
endmacro()

macro (add_test_case_with_run folder _name)
	add_test_executable_with_lib(${folder} ${_name})
	add_test(NAME ${folder}_${_name} COMMAND "./run" "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/test_${folder}_${_name}" WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/")

endmacro()

# Test ROM

add_test_case_with_run(arith zk_rom)
add_test_case_with_run(arith inset_zk_rom)

# Test RAM

add_test_case_with_run(arith zk_ram)

# Test batch optimized ROM

add_test_case_with_run(arith zk_rom_block)
add_test_case_with_run(arith inset_zk_rom_block)

# Test batch optimzed RAM

add_test_case_with_run(arith zk_ram_block)

# Test GOT

add_test_case_with_run(arith GOT)