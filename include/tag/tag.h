/**
 * @file trac.h
 * @brief The top level header file for tag tracker
 **/
#ifndef __TAG_H__
#include <tag/tag_def.h>
#include <tag/tag_set.h>
/**
 * @brief initialization
 * @return < 0 if failed to initialize
 **/
int tag_init();
/**
 * @brief finalization
 * @return nothing
 **/
void tag_finalize();

#endif

