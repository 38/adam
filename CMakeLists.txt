cmake_minimum_required(VERSION 2.6)
project(anadroid)

set(CMAKE_USE_RELATIVE_PATHS ON)

file(MAKE_DIRECTORY bin/lib)
file(MAKE_DIRECTORY bin/test)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY bin/lib)

set(CFLAGS -g\ -DLOG_LEVEL=6\ -DPARSER_COUNT)
set(LDFLAGS \ )


include_directories("include" ".")

aux_source_directory(src/ anadroid_FILES)
aux_source_directory(src/dalvik anadroid_FILES)
aux_source_directory(src/cesk anadroid_FILES)
set_source_files_properties(${anadroid_FILES} PROPERTIES COMPILE_FLAGS ${CFLAGS})
add_library(anadroid ${anadroid_FILES})

file(GLOB UnitTests "test/test_*.c")
foreach(test ${UnitTests})
    get_filename_component(TEST_BIN ${test} NAME_WE)
    set_source_files_properties(${test} PROPERTIES COMPILE_FLAGS ${CFLAGS}) 
    add_executable(test/${TEST_BIN} ${test})
    target_link_libraries(test/${TEST_BIN} anadroid)
endforeach(test)
