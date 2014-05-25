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
 * @brief analyze a method 
 * @param code the code block graph of the method
 * @param frame current stack frame
 * @return a diff for the frame of its caller stack
 **/
cesk_diff_t* cesk_method_analyze(const dalvik_block_t* code, cesk_frame_t* frame);
#endif
