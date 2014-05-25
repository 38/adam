/**
 * @brief the method analyzer
 **/
#ifndef __CESK_METHOD_H__
#define __CESK_METHOD_H__
#include <constants.h>
#include <const_assertion.h>

#include <dalvik/dalvik_block.h>

#include <cesk/cesk_frame.h>
#include <cesk/cesk_diff.h>
#include <cesk/cesk_block.h>
/**
 * @brief initialize the method analyzer
 * @return result of intialization, < 0 indicates errors
 **/
int cesk_method_init();
/**
 * @brief finalize the method analyzer
 * @return nothing
 **/
void cesk_method_finalize();
/**
 * @brief analyze a method 
 * @param code the code block graph of the method
 * @param frame current stack frame
 * @return a diff for the frame of its caller stack
 **/
cesk_diff_t* cesk_method_analyze(const dalvik_block_t* code, cesk_frame_t* frame);
#endif
