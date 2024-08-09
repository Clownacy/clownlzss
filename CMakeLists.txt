cmake_minimum_required(VERSION 3.7.2)

if((${CMAKE_VERSION} VERSION_EQUAL 3.9) OR (${CMAKE_VERSION} VERSION_GREATER 3.9))
	cmake_policy(SET CMP0069 NEW)
endif()

project(clownlzss LANGUAGES C)

# clownlzss

add_library(clownlzss STATIC
	"clownlzss.c"
	"clownlzss.h"
)

set_target_properties(clownlzss PROPERTIES
	C_STANDARD 90
	C_STANDARD_REQUIRED NO
	C_EXTENSIONS OFF
)

# Common

add_library(clownlzss-common STATIC
	"common.c"
	"common.h"
)

set_target_properties(clownlzss-common PROPERTIES
	C_STANDARD 90
	C_STANDARD_REQUIRED NO
	C_EXTENSIONS OFF
)

# Chameleon

add_library(clownlzss-chameleon STATIC
	"chameleon.c"
	"chameleon.h"
)

set_target_properties(clownlzss-chameleon PROPERTIES
	C_STANDARD 90
	C_STANDARD_REQUIRED NO
	C_EXTENSIONS OFF
)

target_link_libraries(clownlzss-chameleon PRIVATE clownlzss clownlzss-common)

# Comper

add_library(clownlzss-comper STATIC
	"comper.c"
	"comper.h"
)

set_target_properties(clownlzss-comper PROPERTIES
	C_STANDARD 90
	C_STANDARD_REQUIRED NO
	C_EXTENSIONS OFF
)

target_link_libraries(clownlzss-comper PRIVATE clownlzss clownlzss-common)

# Faxman

add_library(clownlzss-faxman STATIC
	"faxman.c"
	"faxman.h"
)

set_target_properties(clownlzss-faxman PROPERTIES
	C_STANDARD 90
	C_STANDARD_REQUIRED NO
	C_EXTENSIONS OFF
)

target_link_libraries(clownlzss-faxman PRIVATE clownlzss clownlzss-common)

# Kosinski

add_library(clownlzss-kosinski STATIC
	"kosinski.c"
	"kosinski.h"
)

set_target_properties(clownlzss-kosinski PROPERTIES
	C_STANDARD 90
	C_STANDARD_REQUIRED NO
	C_EXTENSIONS OFF
)

target_link_libraries(clownlzss-kosinski PRIVATE clownlzss clownlzss-common)

# Kosinski+

add_library(clownlzss-kosinskiplus STATIC
	"kosinskiplus.c"
	"kosinskiplus.h"
)

set_target_properties(clownlzss-kosinskiplus PROPERTIES
	C_STANDARD 90
	C_STANDARD_REQUIRED NO
	C_EXTENSIONS OFF
)

target_link_libraries(clownlzss-kosinskiplus PRIVATE clownlzss clownlzss-common)

# Rage

add_library(clownlzss-rage STATIC
	"rage.c"
	"rage.h"
)

set_target_properties(clownlzss-rage PROPERTIES
	C_STANDARD 90
	C_STANDARD_REQUIRED NO
	C_EXTENSIONS OFF
)

target_link_libraries(clownlzss-rage PRIVATE clownlzss clownlzss-common)

# Rocket

add_library(clownlzss-rocket STATIC
	"rocket.c"
	"rocket.h"
)

set_target_properties(clownlzss-rocket PROPERTIES
	C_STANDARD 90
	C_STANDARD_REQUIRED NO
	C_EXTENSIONS OFF
)

target_link_libraries(clownlzss-rocket PRIVATE clownlzss clownlzss-common)

# Saxman

add_library(clownlzss-saxman STATIC
	"saxman.c"
	"saxman.h"
)

set_target_properties(clownlzss-saxman PROPERTIES
	C_STANDARD 90
	C_STANDARD_REQUIRED NO
	C_EXTENSIONS OFF
)

target_link_libraries(clownlzss-saxman PRIVATE clownlzss clownlzss-common)

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

# Chameleon
add_test(NAME test_chameleon_run COMMAND clownlzss-tool -ch "${CMAKE_CURRENT_SOURCE_DIR}/test/clone_driver_v2_dac_driver/uncompressed" "temp")
add_test(NAME test_chameleon_compare COMMAND ${CMAKE_COMMAND} -E compare_files "${CMAKE_CURRENT_SOURCE_DIR}/test/clone_driver_v2_dac_driver/chameleon" "temp")

# Comper
add_test(NAME test_comper_run COMMAND clownlzss-tool -c "${CMAKE_CURRENT_SOURCE_DIR}/test/clone_driver_v2_dac_driver/uncompressed" "temp")
add_test(NAME test_comper_compare COMMAND ${CMAKE_COMMAND} -E compare_files "${CMAKE_CURRENT_SOURCE_DIR}/test/clone_driver_v2_dac_driver/comper" "temp")

# Kosinski
add_test(NAME test_kosinski_run COMMAND clownlzss-tool -k "${CMAKE_CURRENT_SOURCE_DIR}/test/clone_driver_v2_dac_driver/uncompressed" "temp")
add_test(NAME test_kosinski_compare COMMAND ${CMAKE_COMMAND} -E compare_files "${CMAKE_CURRENT_SOURCE_DIR}/test/clone_driver_v2_dac_driver/kosinski" "temp")

# Kosinski+
add_test(NAME test_kosinskiplus_run COMMAND clownlzss-tool -kp "${CMAKE_CURRENT_SOURCE_DIR}/test/clone_driver_v2_dac_driver/uncompressed" "temp")
add_test(NAME test_kosinskiplus_compare COMMAND ${CMAKE_COMMAND} -E compare_files "${CMAKE_CURRENT_SOURCE_DIR}/test/clone_driver_v2_dac_driver/kosinskiplus" "temp")

# Rage
add_test(NAME test_rage_run COMMAND clownlzss-tool -ra "${CMAKE_CURRENT_SOURCE_DIR}/test/clone_driver_v2_dac_driver/uncompressed" "temp")
add_test(NAME test_rage_compare COMMAND ${CMAKE_COMMAND} -E compare_files "${CMAKE_CURRENT_SOURCE_DIR}/test/clone_driver_v2_dac_driver/rage" "temp")

# Rocket
add_test(NAME test_rocket_run COMMAND clownlzss-tool -r "${CMAKE_CURRENT_SOURCE_DIR}/test/clone_driver_v2_dac_driver/uncompressed" "temp")
add_test(NAME test_rocket_compare COMMAND ${CMAKE_COMMAND} -E compare_files "${CMAKE_CURRENT_SOURCE_DIR}/test/clone_driver_v2_dac_driver/rocket" "temp")

# Saxman
add_test(NAME test_saxman_run COMMAND clownlzss-tool -s "${CMAKE_CURRENT_SOURCE_DIR}/test/clone_driver_v2_dac_driver/uncompressed" "temp")
add_test(NAME test_saxman_compare COMMAND ${CMAKE_COMMAND} -E compare_files "${CMAKE_CURRENT_SOURCE_DIR}/test/clone_driver_v2_dac_driver/saxman" "temp")

# Saxman (no header)
add_test(NAME test_saxman_no_header_run COMMAND clownlzss-tool -sn "${CMAKE_CURRENT_SOURCE_DIR}/test/clone_driver_v2_dac_driver/uncompressed" "temp")
add_test(NAME test_saxman_no_header_compare COMMAND ${CMAKE_COMMAND} -E compare_files "${CMAKE_CURRENT_SOURCE_DIR}/test/clone_driver_v2_dac_driver/saxman_no_header" "temp")

# Faxman
add_test(NAME test_faxman_run COMMAND clownlzss-tool -f "${CMAKE_CURRENT_SOURCE_DIR}/test/clone_driver_v2_dac_driver/uncompressed" "temp")
add_test(NAME test_faxman_compare COMMAND ${CMAKE_COMMAND} -E compare_files "${CMAKE_CURRENT_SOURCE_DIR}/test/clone_driver_v2_dac_driver/faxman" "temp")

# Chameleon (moduled)
add_test(NAME test_moduled_chameleon_run COMMAND clownlzss-tool -m -ch "${CMAKE_CURRENT_SOURCE_DIR}/test/clone_driver_v2_dac_driver/uncompressed" "temp")
add_test(NAME test_moduled_chameleon_compare COMMAND ${CMAKE_COMMAND} -E compare_files "${CMAKE_CURRENT_SOURCE_DIR}/test/clone_driver_v2_dac_driver/moduled_chameleon" "temp")

# Comper (moduled)
add_test(NAME test_moduled_comper_run COMMAND clownlzss-tool -m -c "${CMAKE_CURRENT_SOURCE_DIR}/test/clone_driver_v2_dac_driver/uncompressed" "temp")
add_test(NAME test_moduled_comper_compare COMMAND ${CMAKE_COMMAND} -E compare_files "${CMAKE_CURRENT_SOURCE_DIR}/test/clone_driver_v2_dac_driver/moduled_comper" "temp")

# Kosinski (moduled)
add_test(NAME test_moduled_kosinski_run COMMAND clownlzss-tool -m -k "${CMAKE_CURRENT_SOURCE_DIR}/test/clone_driver_v2_dac_driver/uncompressed" "temp")
add_test(NAME test_moduled_kosinski_compare COMMAND ${CMAKE_COMMAND} -E compare_files "${CMAKE_CURRENT_SOURCE_DIR}/test/clone_driver_v2_dac_driver/moduled_kosinski" "temp")

# Kosinski+ (moduled)
add_test(NAME test_moduled_kosinskiplus_run COMMAND clownlzss-tool -m -kp "${CMAKE_CURRENT_SOURCE_DIR}/test/clone_driver_v2_dac_driver/uncompressed" "temp")
add_test(NAME test_moduled_kosinskiplus_compare COMMAND ${CMAKE_COMMAND} -E compare_files "${CMAKE_CURRENT_SOURCE_DIR}/test/clone_driver_v2_dac_driver/moduled_kosinskiplus" "temp")

# Rage (moduled)
add_test(NAME test_moduled_rage_run COMMAND clownlzss-tool -m -ra "${CMAKE_CURRENT_SOURCE_DIR}/test/clone_driver_v2_dac_driver/uncompressed" "temp")
add_test(NAME test_moduled_rage_compare COMMAND ${CMAKE_COMMAND} -E compare_files "${CMAKE_CURRENT_SOURCE_DIR}/test/clone_driver_v2_dac_driver/moduled_rage" "temp")

# Rocket (moduled)
add_test(NAME test_moduled_rocket_run COMMAND clownlzss-tool -m -r "${CMAKE_CURRENT_SOURCE_DIR}/test/clone_driver_v2_dac_driver/uncompressed" "temp")
add_test(NAME test_moduled_rocket_compare COMMAND ${CMAKE_COMMAND} -E compare_files "${CMAKE_CURRENT_SOURCE_DIR}/test/clone_driver_v2_dac_driver/moduled_rocket" "temp")

# Saxman (moduled)
add_test(NAME test_moduled_saxman_run COMMAND clownlzss-tool -m -s "${CMAKE_CURRENT_SOURCE_DIR}/test/clone_driver_v2_dac_driver/uncompressed" "temp")
add_test(NAME test_moduled_saxman_compare COMMAND ${CMAKE_COMMAND} -E compare_files "${CMAKE_CURRENT_SOURCE_DIR}/test/clone_driver_v2_dac_driver/moduled_saxman" "temp")

# Saxman (no header) (moduled)
add_test(NAME test_moduled_saxman_no_header_run COMMAND clownlzss-tool -m -sn "${CMAKE_CURRENT_SOURCE_DIR}/test/clone_driver_v2_dac_driver/uncompressed" "temp")
add_test(NAME test_moduled_saxman_no_header_compare COMMAND ${CMAKE_COMMAND} -E compare_files "${CMAKE_CURRENT_SOURCE_DIR}/test/clone_driver_v2_dac_driver/moduled_saxman_no_header" "temp")

# Faxman (moduled)
add_test(NAME test_moduled_faxman_run COMMAND clownlzss-tool -m -f "${CMAKE_CURRENT_SOURCE_DIR}/test/clone_driver_v2_dac_driver/uncompressed" "temp")
add_test(NAME test_moduled_faxman_compare COMMAND ${CMAKE_COMMAND} -E compare_files "${CMAKE_CURRENT_SOURCE_DIR}/test/clone_driver_v2_dac_driver/moduled_faxman" "temp")

# Comper decompress
add_test(NAME test_comper_decompress_run COMMAND clownlzss-tool -d -c "${CMAKE_CURRENT_SOURCE_DIR}/test/clone_driver_v2_dac_driver/comper" "temp")
add_test(NAME test_comper_decompress_compare COMMAND ${CMAKE_COMMAND} -E compare_files "${CMAKE_CURRENT_SOURCE_DIR}/test/clone_driver_v2_dac_driver/uncompressed" "temp")

# Saxman decompress
add_test(NAME test_saxman_decompress_run COMMAND clownlzss-tool -d -s "${CMAKE_CURRENT_SOURCE_DIR}/test/clone_driver_v2_dac_driver/saxman" "temp")
add_test(NAME test_saxman_decompress_compare COMMAND ${CMAKE_COMMAND} -E compare_files "${CMAKE_CURRENT_SOURCE_DIR}/test/clone_driver_v2_dac_driver/uncompressed" "temp")
