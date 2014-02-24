#ifndef __DALVIK_LOADER_H__
#define __DALVIK_LOADER_H__
/** @file dalvik_loader.h
 *  @brief dalvik package loader
 */
#include <log.h>
/**@brief load from path */
int dalvik_loader_from_directory(const char* path);
/**@brief print a summary */
void dalvik_loader_summary();

#endif
