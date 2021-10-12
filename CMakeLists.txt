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
	C_STANDARD 99
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

add_executable(test_kosinski
	"test/test.c"
	"clownlzss.h"
	"common.c"
	"common.h"
	"kosinski.c"
	"kosinski.h"
	"memory_stream.c"
	"memory_stream.h"
)

add_test(NAME test_kosinski COMMAND test_kosinski)
