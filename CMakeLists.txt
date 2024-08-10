cmake_minimum_required(VERSION 3.7.2)

if((${CMAKE_VERSION} VERSION_EQUAL 3.9) OR (${CMAKE_VERSION} VERSION_GREATER 3.9))
	cmake_policy(SET CMP0069 NEW)
endif()

project(clownlzss LANGUAGES C CXX)

function(make_library name filename)
	add_library(${name} STATIC
		"${filename}.c"
		"${filename}.h"
	)

	set_target_properties(${name} PROPERTIES
		C_STANDARD 90
		C_STANDARD_REQUIRED NO
		C_EXTENSIONS OFF
	)
endfunction()

function(make_compression_library name)
	make_library(clownlzss-${name} ${name})
	target_link_libraries(clownlzss-${name} PRIVATE clownlzss clownlzss-common)
endfunction()

make_library(clownlzss clownlzss)
make_library(clownlzss-common common)
make_compression_library(chameleon)
make_compression_library(comper)
make_compression_library(faxman)
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

# MSVC tweak
if(MSVC)
	target_compile_definitions(clownlzss-tool PRIVATE _CRT_SECURE_NO_WARNINGS)	# Shut up those stupid warnings
endif()


#########
# Tests #
#########

enable_testing()

function(make_compress_test_internal compression-name compression-command)
	foreach(directory "clone_driver_v2_dac_driver" "chameleon_code")
		add_test(NAME test_${directory}_${compression-name}_run COMMAND clownlzss-tool ${compression-command} "${CMAKE_CURRENT_SOURCE_DIR}/test/${directory}/uncompressed" "temp")
		add_test(NAME test_${directory}_${compression-name}_compare COMMAND ${CMAKE_COMMAND} -E compare_files "${CMAKE_CURRENT_SOURCE_DIR}/test/${directory}/${compression-name}" "temp")
	endforeach()
endfunction()

function(make_compress_test compression-name compression-command)
	make_compress_test_internal("${compression-name}" "${compression-command}")
	make_compress_test_internal("moduled_${compression-name}" "-m;${compression-command}")
endfunction()

make_compress_test(chameleon "-ch")
make_compress_test(comper "-c")
make_compress_test(kosinski "-k")
make_compress_test(kosinskiplus "-kp")
make_compress_test(rage "-ra")
make_compress_test(rocket "-r")
make_compress_test(saxman "-s")
make_compress_test(saxman_no_header "-sn")
make_compress_test(faxman "-f")

set_property(TEST test_clone_driver_v2_dac_driver_comper_run PROPERTY WILL_FAIL true)
set_property(TEST test_clone_driver_v2_dac_driver_comper_compare PROPERTY WILL_FAIL true)
set_property(TEST test_clone_driver_v2_dac_driver_moduled_comper_run PROPERTY WILL_FAIL true)
set_property(TEST test_clone_driver_v2_dac_driver_moduled_comper_compare PROPERTY WILL_FAIL true)

function(make_decompress_test compression-name compression-command)
	foreach(directory "clone_driver_v2_dac_driver" "chameleon_code")
		add_test(NAME test_${directory}_${compression-name}_decompress_run COMMAND clownlzss-tool -d ${compression-command} "${CMAKE_CURRENT_SOURCE_DIR}/test/${directory}/${compression-name}" "temp")
		add_test(NAME test_${directory}_${compression-name}_decompress_compare COMMAND ${CMAKE_COMMAND} -E compare_files "${CMAKE_CURRENT_SOURCE_DIR}/test/${directory}/uncompressed" "temp")
	endforeach()
endfunction()

make_decompress_test(comper "-c")
make_decompress_test(kosinski "-k")
make_decompress_test(kosinskiplus "-kp")
make_decompress_test(saxman "-s")

set_property(TEST test_clone_driver_v2_dac_driver_comper_decompress_run PROPERTY WILL_FAIL true)
set_property(TEST test_clone_driver_v2_dac_driver_comper_decompress_compare PROPERTY WILL_FAIL true)
