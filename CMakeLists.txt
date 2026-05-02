cmake_minimum_required(VERSION 3.7.2)

option(CLOWNLZSS_TOOL "Produce compression utility" ON)
option(CLOWNLZSS_COMPRESSORS "Produce compression libraries" ON)

project(clownlzss LANGUAGES C CXX)

include(GNUInstallDirs)

function(make_compression_core_library name filename)
	add_library(${name} STATIC
		"compressors/${filename}.c"
		"compressors/${filename}.h"
	)

	set_target_properties(${name} PROPERTIES
		C_STANDARD 90
		C_STANDARD_REQUIRED NO
		C_EXTENSIONS OFF
	)

	set_target_properties(${name} PROPERTIES PUBLIC_HEADER "compressors/${filename}.h")

	install(TARGETS ${name}
		ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
		LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
		RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
		PUBLIC_HEADER DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/clownlzss/compressors"
	)
endfunction()

function(make_header_library name prefix directory sources)
	add_library(clownlzss-${prefix}${name} INTERFACE
		"${sources}"
	)

	set_target_properties(clownlzss-${prefix}${name} PROPERTIES
		CXX_STANDARD 20
		CXX_STANDARD_REQUIRED NO
		CXX_EXTENSIONS OFF
	)

	set_target_properties(clownlzss-${prefix}${name} PROPERTIES PUBLIC_HEADER "${sources}")

	install(TARGETS clownlzss-${prefix}${name}
		ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
		LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
		RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
		PUBLIC_HEADER DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/clownlzss/${directory}"
	)
endfunction()

make_header_library("common" "" "" "bitfield.h;common.h")

function(make_lone_header_library name prefix directory)
	make_header_library("${name}" "${prefix}" "${directory}" "${directory}${name}.h")
	target_link_libraries(clownlzss-${prefix}${name} INTERFACE clownlzss-common)
endfunction()

function(make_lone_header_libraries name)
	if(CLOWNLZSS_COMPRESSORS)
		# Compression
		make_lone_header_library(${name} "compression-" "compressors/")
	endif()

	# Decompression
	make_lone_header_library(${name} "decompression-" "decompressors/")
endfunction()

make_lone_header_libraries("common")

function(make_format_libraries name)
	make_lone_header_libraries(${name})

	if(CLOWNLZSS_COMPRESSORS)
		target_link_libraries(clownlzss-compression-${name} INTERFACE clownlzss-compression-common clownlzss-compression-core)
	endif()
	target_link_libraries(clownlzss-decompression-${name} INTERFACE clownlzss-decompression-common)
endfunction()

if(CLOWNLZSS_COMPRESSORS)
	make_compression_core_library(clownlzss-compression-core clownlzss)
endif()
make_format_libraries(chameleon)
make_format_libraries(comper)
make_format_libraries(faxman)
make_format_libraries(gba)
make_format_libraries(kosinski)
make_format_libraries(kosinskiplus)
make_format_libraries(rage)
make_format_libraries(rocket)
make_format_libraries(saxman)

if(CLOWNLZSS_TOOL)
	add_executable(clownlzss
		"main.cpp"
	)

	set_target_properties(clownlzss PROPERTIES
		CXX_STANDARD 20
		CXX_STANDARD_REQUIRED NO
		CXX_EXTENSIONS OFF
	)

	target_link_libraries(clownlzss PRIVATE clownlzss-compression-chameleon clownlzss-compression-comper clownlzss-compression-faxman clownlzss-compression-kosinski clownlzss-compression-kosinskiplus clownlzss-compression-rage clownlzss-compression-rocket clownlzss-compression-saxman)
	install(TARGETS clownlzss DESTINATION "${CMAKE_INSTALL_BINDIR}")
endif()


#########
# Tests #
#########

if(CLOWNLZSS_TOOL)
	enable_testing()

	function(make_test_internal compression-name command)
		foreach(directory "clone_driver_v2_dac_driver" "chameleon_code" "executable")
			# Compress
			add_test(NAME ${compression-name}_compress_run_${directory} COMMAND clownlzss ${command} "${CMAKE_CURRENT_SOURCE_DIR}/test/${directory}/uncompressed" "zzzz_${compression-name}_compress_${directory}")
			add_test(NAME ${compression-name}_compress_compare_${directory} COMMAND ${CMAKE_COMMAND} -E compare_files "${CMAKE_CURRENT_SOURCE_DIR}/test/${directory}/${compression-name}" "zzzz_${compression-name}_compress_${directory}")
			set_tests_properties(${compression-name}_compress_compare_${directory} PROPERTIES DEPENDS "${compression-name}_compress_run_${directory}")

			# Decompress
			add_test(NAME ${compression-name}_decompress_run_${directory} COMMAND clownlzss -d ${command} "${CMAKE_CURRENT_SOURCE_DIR}/test/${directory}/${compression-name}" "zzzz_${compression-name}_decompress_${directory}")
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
endif()
