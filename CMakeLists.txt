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

if(0)
make_test(chameleon "-ch")
make_test(comper "-c")
make_test(kosinski "-k")
make_test(kosinskiplus "-kp")
make_test(rage "-ra")
make_test(rocket "-r")
make_test(saxman "-s")
make_test(saxman_no_header "-sn")
make_test(faxman "-f")

set_property(TEST comper_compress_run_clone_driver_v2_dac_driver PROPERTY WILL_FAIL true)
set_property(TEST comper_compress_compare_clone_driver_v2_dac_driver PROPERTY WILL_FAIL true)
set_property(TEST comper_moduled_compress_run_clone_driver_v2_dac_driver PROPERTY WILL_FAIL true)
set_property(TEST comper_moduled_compress_compare_clone_driver_v2_dac_driver PROPERTY WILL_FAIL true)
set_property(TEST comper_decompress_run_clone_driver_v2_dac_driver PROPERTY WILL_FAIL true)
set_property(TEST comper_decompress_compare_clone_driver_v2_dac_driver PROPERTY WILL_FAIL true)
set_property(TEST comper_moduled_decompress_run_clone_driver_v2_dac_driver PROPERTY WILL_FAIL true)
set_property(TEST comper_moduled_decompress_compare_clone_driver_v2_dac_driver PROPERTY WILL_FAIL true)
endif()

cmake_policy(SET CMP0110 NEW)

function(make_enigma_test file_path)
	# Compress
	add_test(NAME "enigma_test_${file_path}_decompress" COMMAND clownlzss-tool -e -d "${file_path}" "test_uncompressed")
	add_test(NAME "enigma_test_${file_path}_compress" COMMAND clownlzss-tool -e "test_uncompressed" "test_compressed")
	add_test(NAME "enigma_test_${file_path}_compare" COMMAND ${CMAKE_COMMAND} -E compare_files "test_compressed" "${file_path}")
#	set_tests_properties(${compression-name}_compress_compare_${directory} PROPERTIES DEPENDS "${compression-name}_compress_run_${directory}")
endfunction()

make_enigma_test("${CMAKE_CURRENT_SOURCE_DIR}/test/pluto-sonic-crackers-thing/EnigmaComp/MapeniSSZ16x16BG.bin")
make_enigma_test("${CMAKE_CURRENT_SOURCE_DIR}/test/pluto-sonic-crackers-thing/EnigmaComp/MapeniSSZ16x16FG.bin")
make_enigma_test("${CMAKE_CURRENT_SOURCE_DIR}/test/pluto-sonic-crackers-thing/EnigmaComp/MapeniSSZ128x128BG.bin")
make_enigma_test("${CMAKE_CURRENT_SOURCE_DIR}/test/pluto-sonic-crackers-thing/EnigmaComp/MapeniSSZ128x128FG.bin")
make_enigma_test("${CMAKE_CURRENT_SOURCE_DIR}/test/pluto-sonic-crackers-thing/EnigmaComp/MapeniSSZLayoutBG.bin")
make_enigma_test("${CMAKE_CURRENT_SOURCE_DIR}/test/pluto-sonic-crackers-thing/EnigmaComp/MapeniSSZLayoutFG.bin")
make_enigma_test("${CMAKE_CURRENT_SOURCE_DIR}/test/pluto-sonic-crackers-thing/EnigmaComp/MapeniTTZ16x16BG.bin")
make_enigma_test("${CMAKE_CURRENT_SOURCE_DIR}/test/pluto-sonic-crackers-thing/EnigmaComp/MapeniTTZ16x16FG.bin")
make_enigma_test("${CMAKE_CURRENT_SOURCE_DIR}/test/pluto-sonic-crackers-thing/EnigmaComp/MapeniTTZ128x128BG.bin")
make_enigma_test("${CMAKE_CURRENT_SOURCE_DIR}/test/pluto-sonic-crackers-thing/EnigmaComp/MapeniTTZ128x128FG.bin")
make_enigma_test("${CMAKE_CURRENT_SOURCE_DIR}/test/pluto-sonic-crackers-thing/EnigmaComp/MapeniTTZLayoutBG.bin")
make_enigma_test("${CMAKE_CURRENT_SOURCE_DIR}/test/pluto-sonic-crackers-thing/EnigmaComp/MapeniTTZLayoutFG.bin")

make_enigma_test("C:/git/s2disasm/mappings/misc/2P Act Results.eni")
make_enigma_test("C:/git/s2disasm/mappings/misc/2P Game Results.eni")
make_enigma_test("C:/git/s2disasm/mappings/misc/2P Special Stage Act Results.eni")
make_enigma_test("C:/git/s2disasm/mappings/misc/2P Special Stage Zone Results.eni")
make_enigma_test("C:/git/s2disasm/mappings/misc/2P Zone Results.eni")
make_enigma_test("C:/git/s2disasm/mappings/misc/Closeup of Sonic flying plane in ending sequence.eni")
make_enigma_test("C:/git/s2disasm/mappings/misc/Closeup of Tails flying plane in ending sequence.eni")
make_enigma_test("C:/git/s2disasm/mappings/misc/End of game sequence frame 1.eni")
make_enigma_test("C:/git/s2disasm/mappings/misc/End of game sequence frame 2.eni")
make_enigma_test("C:/git/s2disasm/mappings/misc/End of game sequence frame 3.eni")
make_enigma_test("C:/git/s2disasm/mappings/misc/End of game sequence frame 4.eni")
make_enigma_test("C:/git/s2disasm/mappings/misc/Level Select 2P.eni")
make_enigma_test("C:/git/s2disasm/mappings/misc/Level Select Icons.eni")
make_enigma_test("C:/git/s2disasm/mappings/misc/Level Select.eni")
make_enigma_test("C:/git/s2disasm/mappings/misc/Lower background mappings for special stage.eni")
make_enigma_test("C:/git/s2disasm/mappings/misc/Main background mappings for special stage.eni")
make_enigma_test("C:/git/s2disasm/mappings/misc/Mappings for title screen background 2.eni")
make_enigma_test("C:/git/s2disasm/mappings/misc/Mappings for title screen background.eni")
make_enigma_test("C:/git/s2disasm/mappings/misc/Options Screen.eni")
make_enigma_test("C:/git/s2disasm/mappings/misc/SEGA mappings.eni")
make_enigma_test("C:/git/s2disasm/mappings/misc/Sonic 2 end of game logo.eni")
make_enigma_test("C:/git/s2disasm/mappings/misc/Sonic and Miles animated background.eni")
make_enigma_test("C:/git/s2disasm/mappings/misc/Sonic the Hedgehog 2 title screen logo mappings.eni")

make_enigma_test("C:/git/s1disasm/tilemaps/Hidden Japanese Credits.eni")
make_enigma_test("C:/git/s1disasm/tilemaps/Sega Logo (JP1).eni")
make_enigma_test("C:/git/s1disasm/tilemaps/Sega Logo.eni")
make_enigma_test("C:/git/s1disasm/tilemaps/SS Background 1.eni")
make_enigma_test("C:/git/s1disasm/tilemaps/SS Background 2.eni")
make_enigma_test("C:/git/s1disasm/tilemaps/Title Screen.eni")

make_enigma_test("C:/git/s1disasm/map16/GHZ.eni")
make_enigma_test("C:/git/s1disasm/map16/LZ.eni")
make_enigma_test("C:/git/s1disasm/map16/MZ.eni")
make_enigma_test("C:/git/s1disasm/map16/SBZ.eni")
make_enigma_test("C:/git/s1disasm/map16/SLZ.eni")
make_enigma_test("C:/git/s1disasm/map16/SYZ.eni")

make_enigma_test("C:/git/s1disasm/sslayout/1.eni")
make_enigma_test("C:/git/s1disasm/sslayout/2.eni")
make_enigma_test("C:/git/s1disasm/sslayout/3.eni")
make_enigma_test("C:/git/s1disasm/sslayout/4.eni")
make_enigma_test("C:/git/s1disasm/sslayout/5 (JP1).eni")
make_enigma_test("C:/git/s1disasm/sslayout/5.eni")
make_enigma_test("C:/git/s1disasm/sslayout/6 (JP1).eni")
make_enigma_test("C:/git/s1disasm/sslayout/6.eni")

make_enigma_test("C:/git/skdisasm/General/Blue Sphere/Enigma Map/Title BG.eni")
make_enigma_test("C:/git/skdisasm/General/Blue Sphere/Enigma Map/Title FG.eni")
make_enigma_test("C:/git/skdisasm/General/Competition Menu/Enigma Map/Level Border.eni")
make_enigma_test("C:/git/skdisasm/General/Competition Menu/Enigma Map/Level Images.eni")
make_enigma_test("C:/git/skdisasm/General/Competition Menu/Enigma Map/Level Names.eni")
make_enigma_test("C:/git/skdisasm/General/Competition Menu/Enigma Map/Menu.eni")
make_enigma_test("C:/git/skdisasm/General/S2Menu/Enigma Map/2P Options.eni")
make_enigma_test("C:/git/skdisasm/General/S2Menu/Enigma Map/Level Select 2P.eni")
make_enigma_test("C:/git/skdisasm/General/S2Menu/Enigma Map/Level Select Icons.eni")
make_enigma_test("C:/git/skdisasm/General/S2Menu/Enigma Map/Level Select.eni")
make_enigma_test("C:/git/skdisasm/General/S2Menu/Enigma Map/Options Screen.eni")
make_enigma_test("C:/git/skdisasm/General/Save Menu/Enigma Map/Menu BG.eni")
make_enigma_test("C:/git/skdisasm/General/Save Menu/Enigma Map/Save Screen Layout.eni")
make_enigma_test("C:/git/skdisasm/General/Special Stage/Enigma Map/BG.eni")
make_enigma_test("C:/git/skdisasm/General/Special Stage/Enigma Map/Layout.eni")
