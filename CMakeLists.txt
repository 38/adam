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

if(APPLE)
	set(SYSNAME "OS_X")
elseif(WIN32)
	set(SYSNAME WIN32)
else(APPLE)
	set(SYSNAME ${CMAKE_SYSTEM_NAME})
endif(APPLE)

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
aux_source_directory(src/tag adam_FILES)
set_source_files_properties(${adam_FILES} PROPERTIES COMPILE_FLAGS ${CFLAGS})

if("${STATIC}" STREQUAL "yes")
	add_library(adam ${adam_FILES})
else("${STATIC}" STREQUAL "yes")
	add_library(adam SHARED ${adam_FILES})
endif("${STATIC}" STREQUAL "yes")
	
add_library(adam_dbg ${adam_FILES})

set_target_properties(adam_dbg PROPERTIES COMPILE_DEFINITIONS "DEBUGGER")

set(package_status "" )
set(CONF "System: ${SYSNAME}\nCC=${CMAKE_C_COMPILER}\nCFLAGS=${COMPILE_FLAGS} ${CFLAGS}")
file(GLOB tools RELATIVE ${CMAKE_CURRENT_BINARY_DIR}/${tools_DIR} "${tools_DIR}/*")
foreach(tool ${tools})
	if(NOT "${build_${tool}}" STREQUAL "no")
		if(IS_DIRECTORY ${tools_DIR}/${tool})
			set(tool_FILES "" )
			set(LOCAL_CFLAGS )
			set(LOCAL_LIBS )
			set(TYPE "binary")
			include(${tools_DIR}/${tool}/build.cmake)
			message("I'm gonna build package ${tool} (TYPE = ${TYPE})")
			set(package_status "${package_status} -Dbuild_${tool}=yes")
			if("${TYPE}" STREQUAL "binary")
				aux_source_directory(${tools_DIR}/${tool} tool_FILES)
				set_source_files_properties(${tool_FILES} PROPERTIES COMPILE_FLAGS "${CFLAGS} ${LOCAL_CFLAGS}")
				add_executable(${tool} ${tool_FILES})
				target_link_libraries(${tool} ${LOCAL_LIBS})
			endif("${TYPE}" STREQUAL "binary")
		endif(IS_DIRECTORY ${tools_DIR}/${tool})
	else(NOT "${build_${tool}}" STREQUAL "no")
		set(package_status "${package_status} -Dbuild_${tool}=no")
		message("Skipped package ${tool}")
	endif(NOT "${build_${tool}}" STREQUAL "no")
	set(CONF "${CONF}\nbuild ${tool}: ${build_${tool}}")
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

add_custom_target(get-d2s
	COMMAND test ! -e test/dex2sex &&
	        wget 'http://www.cs.utah.edu/~haohou/adam/dex2sex.tar.gz' --output-document=adam_dex2sex_dasm.tar.gz &&
	        tar -xzf adam_dex2sex_dasm.tar.gz -C test/ &&
	        rm -f adam_dex2sex_dasm.tar.gz ||
			echo > /dev/null
	WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)
#build the test class from java source code
file(GLOB JavaCases "test/java/*")

foreach(case ${JavaCases})
	get_filename_component(PACKAGE_NAME ${case} NAME_WE)
	set(TARGET_NAME _java_case_${PACKAGE_NAME})
	add_custom_target(${TARGET_NAME} 
		DEPENDS get-d2s
		COMMAND test/dex2sex/script/java2s ${case} test/generated/${PACKAGE_NAME}
	)
	list(APPEND DATADEP ${TARGET_NAME})
endforeach(case ${JavaCases})

add_custom_target(data
	DEPENDS ${DATADEP}
	COMMAND test ! -e test/data/AndroidAntlr &&  
	        wget 'http://www.cs.utah.edu/~haohou/adam/data.tar.gz' --output-document=adam_testing_data.tar.gz &&
	        tar -xzf adam_testing_data.tar.gz &&
	        rm -f adam_testing_data.tar.gz ||
			echo > /dev/null
	WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)


find_package(Doxygen)
add_custom_target(docs
	${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile
	WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
	COMMENT "Generating API documentation with Doxygen" VERBATIM
)

add_custom_target(distclean
	COMMAND make clean
	COMMAND rm -rf doc/doxygen
	COMMAND rm -rf test/data
	COMMAND rm -rf tags
	COMMAND rm -rf test/dex2sex
	COMMAND rm -rf test/generated
	WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)

add_custom_target(tags
	COMMAND ctags -R include/ src/ tools/
	WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)

add_custom_target(show-flags
	COMMAND echo "Compiler = ${CMAKE_C_COMPILER}" && 
	        echo "Log Level = ${LOG}" &&
			echo "Optimaization = ${OPTLEVEL}" &&
			echo "Compiler Flags = ${CFLAGS}" &&
			echo "Static Library = ${STATIC}" &&
			echo "L=${LOG} O=${OPTLEVEL} CC=${CMAKE_C_COMPILER} cmake ${package_status} . ")
message("--------------------------------------------------------")
message(${CONF})
message("--------------------------------------------------------")
