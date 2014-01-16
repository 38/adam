cmake_minimum_required(VERSION 2.6)
project(anadroid)


set(CFLAGS -g\ -DLOG_LEVEL=${glimpse_log_level}\ -DENABLE_PROFILER\ -DGLIMPSE_DEBUG)
set(LDFLAGS \ )
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY bin)

file(MAKE_DIRECTORY bin/test)

include_directories("include" ".")

aux_source_directory(src/ anadroid_FILES)
aux_source_directory(src/dalvik anadroid_FILES)
set_source_files_properties(${anadroid_FILES} PROPERTIES COMPILE_FLAGS ${CFLAGS})
add_library(anadroid ${anadroid_FILES})

file(GLOB UnitTests "test/test_*.c")
foreach(test ${UnitTests})
    get_filename_component(TEST_BIN ${test} NAME_WE)
    set_source_files_properties(${test} PROPERTIES COMPILE_FLAGS ${CFLAGS}) 
    add_executable(test/${TEST_BIN} ${test})
    target_link_libraries(test/${TEST_BIN} anadroid)
endforeach(test)
