cmake_minimum_required(VERSION 3.7.2)

if((${CMAKE_VERSION} VERSION_EQUAL 3.9) OR (${CMAKE_VERSION} VERSION_GREATER 3.9))
	cmake_policy(SET CMP0069 NEW)
endif()

project(clownlzss LANGUAGES C)

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
	"main.c"
)

set_target_properties(clownlzss-tool PROPERTIES
	C_STANDARD 90
	C_STANDARD_REQUIRED NO
	C_EXTENSIONS OFF
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
	add_test(NAME test_${compression-name}_run COMMAND clownlzss-tool ${compression-command} "${CMAKE_CURRENT_SOURCE_DIR}/test/clone_driver_v2_dac_driver/uncompressed" "temp")
	add_test(NAME test_${compression-name}_compare COMMAND ${CMAKE_COMMAND} -E compare_files "${CMAKE_CURRENT_SOURCE_DIR}/test/clone_driver_v2_dac_driver/${compression-name}" "temp")
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

function(make_decompress_test compression-name compression-command)
	add_test(NAME test_${compression-name}_decompress_run COMMAND clownlzss-tool -d ${compression-command} "${CMAKE_CURRENT_SOURCE_DIR}/test/clone_driver_v2_dac_driver/${compression-name}" "temp")
	add_test(NAME test_${compression-name}_decompress_compare COMMAND ${CMAKE_COMMAND} -E compare_files "${CMAKE_CURRENT_SOURCE_DIR}/test/clone_driver_v2_dac_driver/uncompressed" "temp")
endfunction()

make_decompress_test(comper "-c")
make_decompress_test(saxman "-s")
