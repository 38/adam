cmake_minimum_required(VERSION 2.6)
project(anadroid)


set(CFLAGS -g\ -DLOG_LEVEL=${glimpse_log_level}\ -DENABLE_PROFILER\ -DGLIMPSE_DEBUG)
set(LDFLAGS \ )

include_directories("include" ".")

aux_source_directory(src/ anadroid_FILES)
aux_source_directory(src/dalvik anadroid_FILES)
set_source_files_properties(${anadroid_FILES} PROPERTIES COMPILE_FLAGS ${CFLAGS})
add_library(anadroid ${anadroid_FILES})
