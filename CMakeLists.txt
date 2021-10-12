cmake_minimum_required(VERSION 3.7.2)

if((${CMAKE_VERSION} VERSION_EQUAL 3.9) OR (${CMAKE_VERSION} VERSION_GREATER 3.9))
	cmake_policy(SET CMP0069 NEW)
endif()

project(clownlzss LANGUAGES C)

add_executable(tool
	"chameleon.c"
	"chameleon.h"
	"clownlzss.h"
	"common.c"
	"common.h"
	"comper.c"
	"comper.h"
	"faxman.c"
	"faxman.h"
	"kosinski.c"
	"kosinski.h"
	"kosinskiplus.c"
	"kosinskiplus.h"
	"main.c"
	"memory_stream.c"
	"memory_stream.h"
	"rage.c"
	"rage.h"
	"rocket.c"
	"rocket.h"
	"saxman.c"
	"saxman.h"
)

set_target_properties(tool PROPERTIES
	C_STANDARD 90
	C_EXTENSIONS OFF
)

# MSVC tweak
if(MSVC)
	target_compile_definitions(tool PRIVATE _CRT_SECURE_NO_WARNINGS)	# Shut up those stupid warnings
endif()


#########
# Tests #
#########

enable_testing()

# Chameleon
add_test(NAME test_chameleon_run COMMAND tool -ch "${CMAKE_CURRENT_SOURCE_DIR}/test/clone_driver_v2_dac_driver/uncompressed" "temp")
add_test(NAME test_chameleon_compare COMMAND ${CMAKE_COMMAND} -E compare_files "${CMAKE_CURRENT_SOURCE_DIR}/test/clone_driver_v2_dac_driver/chameleon" "temp")

# Comper
add_test(NAME test_comper_run COMMAND tool -c "${CMAKE_CURRENT_SOURCE_DIR}/test/clone_driver_v2_dac_driver/uncompressed" "temp")
add_test(NAME test_comper_compare COMMAND ${CMAKE_COMMAND} -E compare_files "${CMAKE_CURRENT_SOURCE_DIR}/test/clone_driver_v2_dac_driver/comper" "temp")

# Kosinski
add_test(NAME test_kosinski_run COMMAND tool -k "${CMAKE_CURRENT_SOURCE_DIR}/test/clone_driver_v2_dac_driver/uncompressed" "temp")
add_test(NAME test_kosinski_compare COMMAND ${CMAKE_COMMAND} -E compare_files "${CMAKE_CURRENT_SOURCE_DIR}/test/clone_driver_v2_dac_driver/kosinski" "temp")

# Kosinski+
add_test(NAME test_kosinskiplus_run COMMAND tool -kp "${CMAKE_CURRENT_SOURCE_DIR}/test/clone_driver_v2_dac_driver/uncompressed" "temp")
add_test(NAME test_kosinskiplus_compare COMMAND ${CMAKE_COMMAND} -E compare_files "${CMAKE_CURRENT_SOURCE_DIR}/test/clone_driver_v2_dac_driver/kosinskiplus" "temp")

# Rage
add_test(NAME test_rage_run COMMAND tool -ra "${CMAKE_CURRENT_SOURCE_DIR}/test/clone_driver_v2_dac_driver/uncompressed" "temp")
add_test(NAME test_rage_compare COMMAND ${CMAKE_COMMAND} -E compare_files "${CMAKE_CURRENT_SOURCE_DIR}/test/clone_driver_v2_dac_driver/rage" "temp")

# Rocket
add_test(NAME test_rocket_run COMMAND tool -r "${CMAKE_CURRENT_SOURCE_DIR}/test/clone_driver_v2_dac_driver/uncompressed" "temp")
add_test(NAME test_rocket_compare COMMAND ${CMAKE_COMMAND} -E compare_files "${CMAKE_CURRENT_SOURCE_DIR}/test/clone_driver_v2_dac_driver/rocket" "temp")

# Saxman
add_test(NAME test_saxman_run COMMAND tool -s "${CMAKE_CURRENT_SOURCE_DIR}/test/clone_driver_v2_dac_driver/uncompressed" "temp")
add_test(NAME test_saxman_compare COMMAND ${CMAKE_COMMAND} -E compare_files "${CMAKE_CURRENT_SOURCE_DIR}/test/clone_driver_v2_dac_driver/saxman" "temp")

# Saxman (no header)
add_test(NAME test_saxman_no_header_run COMMAND tool -sn "${CMAKE_CURRENT_SOURCE_DIR}/test/clone_driver_v2_dac_driver/uncompressed" "temp")
add_test(NAME test_saxman_no_header_compare COMMAND ${CMAKE_COMMAND} -E compare_files "${CMAKE_CURRENT_SOURCE_DIR}/test/clone_driver_v2_dac_driver/saxman_no_header" "temp")

# Faxman
add_test(NAME test_faxman_run COMMAND tool -f "${CMAKE_CURRENT_SOURCE_DIR}/test/clone_driver_v2_dac_driver/uncompressed" "temp")
add_test(NAME test_faxman_compare COMMAND ${CMAKE_COMMAND} -E compare_files "${CMAKE_CURRENT_SOURCE_DIR}/test/clone_driver_v2_dac_driver/faxman" "temp")

# Chameleon (moduled)
add_test(NAME test_moduled_chameleon_run COMMAND tool -m -ch "${CMAKE_CURRENT_SOURCE_DIR}/test/clone_driver_v2_dac_driver/uncompressed" "temp")
add_test(NAME test_moduled_chameleon_compare COMMAND ${CMAKE_COMMAND} -E compare_files "${CMAKE_CURRENT_SOURCE_DIR}/test/clone_driver_v2_dac_driver/moduled_chameleon" "temp")

# Comper (moduled)
add_test(NAME test_moduled_comper_run COMMAND tool -m -c "${CMAKE_CURRENT_SOURCE_DIR}/test/clone_driver_v2_dac_driver/uncompressed" "temp")
add_test(NAME test_moduled_comper_compare COMMAND ${CMAKE_COMMAND} -E compare_files "${CMAKE_CURRENT_SOURCE_DIR}/test/clone_driver_v2_dac_driver/moduled_comper" "temp")

# Kosinski (moduled)
add_test(NAME test_moduled_kosinski_run COMMAND tool -m -k "${CMAKE_CURRENT_SOURCE_DIR}/test/clone_driver_v2_dac_driver/uncompressed" "temp")
add_test(NAME test_moduled_kosinski_compare COMMAND ${CMAKE_COMMAND} -E compare_files "${CMAKE_CURRENT_SOURCE_DIR}/test/clone_driver_v2_dac_driver/moduled_kosinski" "temp")

# Kosinski+ (moduled)
add_test(NAME test_moduled_kosinskiplus_run COMMAND tool -m -kp "${CMAKE_CURRENT_SOURCE_DIR}/test/clone_driver_v2_dac_driver/uncompressed" "temp")
add_test(NAME test_moduled_kosinskiplus_compare COMMAND ${CMAKE_COMMAND} -E compare_files "${CMAKE_CURRENT_SOURCE_DIR}/test/clone_driver_v2_dac_driver/moduled_kosinskiplus" "temp")

# Rage (moduled)
add_test(NAME test_moduled_rage_run COMMAND tool -m -ra "${CMAKE_CURRENT_SOURCE_DIR}/test/clone_driver_v2_dac_driver/uncompressed" "temp")
add_test(NAME test_moduled_rage_compare COMMAND ${CMAKE_COMMAND} -E compare_files "${CMAKE_CURRENT_SOURCE_DIR}/test/clone_driver_v2_dac_driver/moduled_rage" "temp")

# Rocket (moduled)
add_test(NAME test_moduled_rocket_run COMMAND tool -m -r "${CMAKE_CURRENT_SOURCE_DIR}/test/clone_driver_v2_dac_driver/uncompressed" "temp")
add_test(NAME test_moduled_rocket_compare COMMAND ${CMAKE_COMMAND} -E compare_files "${CMAKE_CURRENT_SOURCE_DIR}/test/clone_driver_v2_dac_driver/moduled_rocket" "temp")

# Saxman (moduled)
add_test(NAME test_moduled_saxman_run COMMAND tool -m -s "${CMAKE_CURRENT_SOURCE_DIR}/test/clone_driver_v2_dac_driver/uncompressed" "temp")
add_test(NAME test_moduled_saxman_compare COMMAND ${CMAKE_COMMAND} -E compare_files "${CMAKE_CURRENT_SOURCE_DIR}/test/clone_driver_v2_dac_driver/moduled_saxman" "temp")

# Saxman (no header) (moduled)
add_test(NAME test_moduled_saxman_no_header_run COMMAND tool -m -sn "${CMAKE_CURRENT_SOURCE_DIR}/test/clone_driver_v2_dac_driver/uncompressed" "temp")
add_test(NAME test_moduled_saxman_no_header_compare COMMAND ${CMAKE_COMMAND} -E compare_files "${CMAKE_CURRENT_SOURCE_DIR}/test/clone_driver_v2_dac_driver/moduled_saxman_no_header" "temp")

# Faxman (moduled)
add_test(NAME test_moduled_faxman_run COMMAND tool -m -f "${CMAKE_CURRENT_SOURCE_DIR}/test/clone_driver_v2_dac_driver/uncompressed" "temp")
add_test(NAME test_moduled_faxman_compare COMMAND ${CMAKE_COMMAND} -E compare_files "${CMAKE_CURRENT_SOURCE_DIR}/test/clone_driver_v2_dac_driver/moduled_faxman" "temp")
