cmake_minimum_required(VERSION 2.6)
include(CheckTypeSize)
project(adam)


check_type_size("void*" SIZE_OF_PTR)

if(SIZE_OF_PTR EQUAL 8)
    set(PTR_T uint64_t)
elseif(SIZE_OF_PTR EQUAL 4)
    set(PTR_T uint32_t)
endif(SIZE_OF_PTR EQUAL 8)

set(CMAKE_USE_RELATIVE_PATHS ON)

file(MAKE_DIRECTORY bin/lib)
file(MAKE_DIRECTORY bin/test)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY bin/lib)
set(tools_DIR "tools")

set(LOG     6)  # log level can be 0 to 6, 0 means no log, 6 means the most enable debug log 
#set(CFLAGS -O0\ -Werror\ -Wall\ -g\ -DLOG_LEVEL=${LOG}\ -DPARSER_COUNT)
set(CFLAGS -O0\ -Wall\ -Werror\ -g\ -DLOG_LEVEL=${LOG}\ -DPARSER_COUNT)
set(LDFLAGS \ )


include_directories("include" ".")

configure_file("config.h.in" "config.h")
enable_testing()

aux_source_directory(src/ adam_FILES)
aux_source_directory(src/dalvik adam_FILES)
aux_source_directory(src/cesk adam_FILES)
set_source_files_properties(${adam_FILES} PROPERTIES COMPILE_FLAGS ${CFLAGS})
add_library(adam ${adam_FILES})


file(GLOB tools RELATIVE ${CMAKE_CURRENT_BINARY_DIR}/${tools_DIR} "${tools_DIR}/*")
foreach(tool ${tools})
	if(IS_DIRECTORY ${tools_DIR}/${tool})
		set(tool_FILES "" )
		aux_source_directory(${tools_DIR}/${tool} tool_FILES)
		set_source_files_properties(${tools_FILES} PROPERTIES COMPILE_FLAGS ${CFLAGS})
		add_executable(${tool} ${tool_FILES})
		target_link_libraries(${tool} adam)
	endif(IS_DIRECTORY ${tools_DIR}/${tool})
endforeach(tool ${tools})

file(GLOB UnitTests "test/test_*.c")
foreach(test ${UnitTests})
    get_filename_component(TEST_BIN ${test} NAME_WE)
    set_source_files_properties(${test} PROPERTIES COMPILE_FLAGS ${CFLAGS}) 
    add_executable(test/${TEST_BIN} ${test})
    target_link_libraries(test/${TEST_BIN} adam)
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
