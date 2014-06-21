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
 * @param caller_ctx the context for caller
 * @param rtab the relocation table
 * @return a diff for the frame of its caller stack
 **/
cesk_diff_t* cesk_method_analyze(const dalvik_block_t* code, cesk_frame_t* frame, const void* caller_ctx, cesk_reloc_table_t** p_rtab);
/**
 * @brief print backtrace in the log
 * @param method_context the method context
 * @return nothing
 **/
void cesk_method_print_backtrace(const void* method_context);

/**
 * @brief get the input frame of this context
 * @param context
 * @return the input frame
 **/
const cesk_frame_t* cesk_method_context_get_input_frame(const void* context);

/**
 * @brief get the code block of this context current executing
 * @param context
 * @return the dalvik code block
 **/
const dalvik_block_t* cesk_method_context_get_current_block(const void* context);

/**
 * @brief get the caller context
 * @param context
 * @return the caller context, NULL means error or the bottom of the stack
 **/
const void* cesk_method_context_get_caller_context(const void* context);
#endif
