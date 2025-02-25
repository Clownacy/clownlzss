cmake_minimum_required(VERSION 3.7.2)

project(clownlzss LANGUAGES C CXX)

function(make_library name filename)
	add_library(${name} STATIC
		"compressors/${filename}.c"
		"compressors/${filename}.h"
	)

	set_target_properties(${name} PROPERTIES
		C_STANDARD 90
		C_STANDARD_REQUIRED NO
		C_EXTENSIONS OFF
	)
endfunction()

function(make_compression_library name)
	add_library(clownlzss-${name} INTERFACE
		"compressors/${name}.h"
	)

	set_target_properties(clownlzss-${name} PROPERTIES
		CXX_STANDARD 20
		CXX_STANDARD_REQUIRED NO
		CXX_EXTENSIONS OFF
	)

	target_link_libraries(clownlzss-${name} INTERFACE clownlzss)
endfunction()

make_library(clownlzss clownlzss)
make_compression_library(chameleon)
make_compression_library(comper)
make_compression_library(faxman)
make_compression_library(gba)
make_compression_library(kosinski)
make_compression_library(kosinskiplus)
make_compression_library(rage)
make_compression_library(rocket)
make_compression_library(saxman)

add_executable(clownlzss-tool
	"main.cpp"
)

set_target_properties(clownlzss-tool PROPERTIES
	CXX_STANDARD 20
	CXX_STANDARD_REQUIRED NO
	CXX_EXTENSIONS OFF
)

target_link_libraries(clownlzss-tool PRIVATE clownlzss-chameleon clownlzss-comper clownlzss-faxman clownlzss-kosinski clownlzss-kosinskiplus clownlzss-rage clownlzss-rocket clownlzss-saxman)
install(TARGETS clownlzss-tool DESTINATION bin)


#########
# Tests #
#########

enable_testing()

function(make_test_internal compression-name command)
	foreach(directory "clone_driver_v2_dac_driver" "chameleon_code" "executable")
		# Compress
		add_test(NAME ${compression-name}_compress_run_${directory} COMMAND clownlzss-tool ${command} "${CMAKE_CURRENT_SOURCE_DIR}/test/${directory}/uncompressed" "zzzz_${compression-name}_compress_${directory}")
		add_test(NAME ${compression-name}_compress_compare_${directory} COMMAND ${CMAKE_COMMAND} -E compare_files "${CMAKE_CURRENT_SOURCE_DIR}/test/${directory}/${compression-name}" "zzzz_${compression-name}_compress_${directory}")
		set_tests_properties(${compression-name}_compress_compare_${directory} PROPERTIES DEPENDS "${compression-name}_compress_run_${directory}")

		# Decompress
		add_test(NAME ${compression-name}_decompress_run_${directory} COMMAND clownlzss-tool -d ${command} "${CMAKE_CURRENT_SOURCE_DIR}/test/${directory}/${compression-name}" "zzzz_${compression-name}_decompress_${directory}")
		add_test(NAME ${compression-name}_decompress_compare_${directory} COMMAND ${CMAKE_COMMAND} -E compare_files "${CMAKE_CURRENT_SOURCE_DIR}/test/${directory}/uncompressed" "zzzz_${compression-name}_decompress_${directory}")
		set_tests_properties(${compression-name}_decompress_compare_${directory} PROPERTIES DEPENDS "${compression-name}_decompress_run_${directory}")
	endforeach()
endfunction()

function(make_test compression-name compression-command)
	make_test_internal("${compression-name}" "${compression-command}")
	make_test_internal("${compression-name}_moduled" "-m;${compression-command}")
endfunction()

make_test(chameleon "-ch")
make_test(comper "-c")
make_test(kosinski "-k")
make_test(kosinskiplus "-kp")
make_test(rage "-ra")
make_test(rocket "-r")
make_test(saxman "-s")
make_test(saxman_no_header "-sn")
make_test(faxman "-f")
make_test(gba "-g")
make_test(gba_vram_safe "-gv")

set_property(TEST comper_compress_run_clone_driver_v2_dac_driver PROPERTY WILL_FAIL true)
set_property(TEST comper_compress_compare_clone_driver_v2_dac_driver PROPERTY WILL_FAIL true)
set_property(TEST comper_moduled_compress_run_clone_driver_v2_dac_driver PROPERTY WILL_FAIL true)
set_property(TEST comper_moduled_compress_compare_clone_driver_v2_dac_driver PROPERTY WILL_FAIL true)
set_property(TEST comper_decompress_run_clone_driver_v2_dac_driver PROPERTY WILL_FAIL true)
set_property(TEST comper_decompress_compare_clone_driver_v2_dac_driver PROPERTY WILL_FAIL true)
set_property(TEST comper_moduled_decompress_run_clone_driver_v2_dac_driver PROPERTY WILL_FAIL true)
set_property(TEST comper_moduled_decompress_compare_clone_driver_v2_dac_driver PROPERTY WILL_FAIL true)
