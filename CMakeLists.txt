cmake_minimum_required(VERSION 2.6)
include(CheckTypeSize)
project(adam)


check_type_size("void*" SIZE_OF_PTR)

if(SIZE_OF_PTR EQUAL 8)
    set(PTR_T uint64_t)
    set(WIDTH 64)
elseif(SIZE_OF_PTR EQUAL 4)
    set(PTR_T uint32_t)
    set(WIDTH 32)
endif(SIZE_OF_PTR EQUAL 8)

set(CMAKE_USE_RELATIVE_PATHS ON)

file(MAKE_DIRECTORY bin/lib)
file(MAKE_DIRECTORY bin/test)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY bin/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY bin/lib)
set(tools_DIR "tools")
set(test_DIR "test")

# log level can be 0 to 6, 0 means no log, 6 means the most detialed log
if("$ENV{L}" STREQUAL "") 
	set(LOG     6) 
else("$ENV{L}" STREQUAL "")
	set(LOG		$ENV{L})
endif("$ENV{L}" STREQUAL "")
if("$ENV{O}" STREQUAL "")
	set(OPTLEVEL	0)
else("$ENV{O}" STREQUAL "")
	set(OPTLEVEL	$ENV{O})
endif("$ENV{O}" STREQUAL "")

if(NOT "$ENV{CC}" STREQUAL "")
	set(CMAKE_C_COMPILER "$ENV{CC}")
endif(NOT "$ENV{CC}" STREQUAL "")

message("Compiler: ${CMAKE_C_COMPILER}")
message("Log Level: ${LOG}")
message("Optimization Level: ${OPTLEVEL}")
set(CFLAGS -O${OPTLEVEL}\ -Wall\ -Werror\ -g\ -DLOG_LEVEL=${LOG}\ -DPARSER_COUNT)


include_directories("include" ".")

configure_file("config.h.in" "config.h")
enable_testing()

aux_source_directory(src/ adam_FILES)
aux_source_directory(src/dalvik adam_FILES)
aux_source_directory(src/cesk adam_FILES)
aux_source_directory(src/bci adam_FILES)
aux_source_directory(src/builtins adam_FILES)
set_source_files_properties(${adam_FILES} PROPERTIES COMPILE_FLAGS ${CFLAGS})
add_library(adam ${adam_FILES})

add_library(adam_dbg ${adam_FILES})
set_target_properties(adam_dbg PROPERTIES COMPILE_DEFINITIONS "DEBUGGER")


file(GLOB tools RELATIVE ${CMAKE_CURRENT_BINARY_DIR}/${tools_DIR} "${tools_DIR}/*")
foreach(tool ${tools})
	if(NOT "${build_${tool}}" STREQUAL "no")
		if(IS_DIRECTORY ${tools_DIR}/${tool})
			set(tool_FILES "" )
			aux_source_directory(${tools_DIR}/${tool} tool_FILES)
			include(${tools_DIR}/${tool}/build.cmake)
			set_source_files_properties(${tool_FILES} PROPERTIES COMPILE_FLAGS "${CFLAGS} ${LOCAL_CFLAGS}")
			add_executable(${tool} ${tool_FILES})
			target_link_libraries(${tool} ${LOCAL_LIBS})
		endif(IS_DIRECTORY ${tools_DIR}/${tool})
	else(NOT "${build_${tool}}" STREQUAL "no")
		message("Skipped package ${tool}")
	endif(NOT "${build_${tool}}" STREQUAL "no")
endforeach(tool ${tools})


file(GLOB UnitTests "test/test_*.c")
foreach(test ${UnitTests})
    get_filename_component(TEST_BIN ${test} NAME_WE)
    set_source_files_properties(${test} PROPERTIES COMPILE_FLAGS ${CFLAGS}) 
    add_executable(${TEST_BIN} ${test})
    set_target_properties(${TEST_BIN} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${test_DIR})
    target_link_libraries(${TEST_BIN} adam)
	add_test(${TEST_BIN} bin/test/${TEST_BIN})
endforeach(test)

find_package(Doxygen)
add_custom_target(docs
	${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile
	WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
	COMMENT "Generating API documentation with Doxygen" VERBATIM
)

add_custom_target(distclean
	COMMAND make clean
	COMMAND rm -rvf doc/doxygen
	COMMAND rm -rvf test/data
	COMMAND rm -rvf tags
	WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)

add_custom_target(data
	COMMAND wget http://www.cs.utah.edu/~haohou/adam/data.tar.gz --output-document=adam_testing_data.tar.gz
	COMMAND tar -xzf adam_testing_data.tar.gz
	COMMAND rm -f adam_testing_data.tar.gz
	WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)

add_custom_target(tags
	COMMAND ctags -R include/ src/
	WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)

add_custom_target(show-flags
	COMMAND echo "Compiler = ${CMAKE_C_COMPILER}" && 
	        echo "Log Level = ${LOG}" &&
			echo "Optimaization = ${OPTLEVEL}" &&
			echo "Compiler Flags = ${CFLAGS}")
