cmake_minimum_required(VERSION 2.6)
project(anadroid)

set(CMAKE_USE_RELATIVE_PATHS ON)

file(MAKE_DIRECTORY bin/lib)
file(MAKE_DIRECTORY bin/test)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY bin/lib)

set(LOG     6)  # log level can be 0 to 6, 0 means no log, 6 means the most enable debug log 
set(CFLAGS -O0\ -Wall\ -g\ -DLOG_LEVEL=${LOG}\ -DPARSER_COUNT)
set(LDFLAGS \ )


include_directories("include" ".")

aux_source_directory(src/ adam_FILES)
aux_source_directory(src/dalvik adam_FILES)
aux_source_directory(src/cesk adam_FILES)
set_source_files_properties(${adam_FILES} PROPERTIES COMPILE_FLAGS ${CFLAGS})
add_library(adam ${adam_FILES})

file(GLOB UnitTests "test/test_*.c")
foreach(test ${UnitTests})
    get_filename_component(TEST_BIN ${test} NAME_WE)
    set_source_files_properties(${test} PROPERTIES COMPILE_FLAGS ${CFLAGS}) 
    add_executable(test/${TEST_BIN} ${test})
    target_link_libraries(test/${TEST_BIN} adam)
endforeach(test)
