/**
 * @brief tracker the value tags
 * @file tag_tracker.h
 **/
#ifndef __TAG_TRACKER_H__
#define __TAG_TRACKER_H__
#include <tag/tag_set.h>
/**
 * @brief a data path
 **/
typedef struct {
	uint32_t instruction;   /*!< current instruction index */
	uint32_t reason_code;   /*!< the reason why this tag transfer happends */
} tag_tracker_path_t;
/**
 * @brief initialize
 * @param < 0 for failure
 **/
int tag_tracker_init();
/**
 * @biref fianlize
 * @return nothing
 **/
void tag_tracker_finalize();
/**
 * @brief add an invocation edge to tag tracker
 * @param tag_set 
 * @param caller_closure the caller closure_id
 * @param invoke_inst the id of invocation instruction 
 * @param callee_closure the callee closure_id
 * @param regidx the register involved
 * @return < 0  if error
 **/
int tag_tracker_invocation(const tag_set_t* tag_set, uint32_t caller_closure, uint32_t invoke_inst, uint32_t callee_closure, uint32_t regidx);
/**
 * @brief add an instruction execution edge to tag tracker
 * @param tag_set
 * @param closure the closure where the instruction execution happends
 * @param to the instruction id to which this tag_set goes
 * @return < 0 if error
 **/
int tag_tracker_execution(const tag_set_t* tag_set, uint32_t closure, uint32_t to);
/**
 * @brief add a block transition edge to tag tracker
 * @param tag_set
 * @param closure the closure where block transition happends
 * @param from the block id from which this tag_set comes
 * @param to the block id to which this tag_set goes
 * @return < 0 if error
 **/
int tag_tacker_transition(const tag_set_t* tag_set, uint32_t closure, uint32_t from, uint32_t to);
/**
 * @brief get data-flow path
 * @param tag_id the tag id to focus on
 * @param sink_closure the closure where the value tag goes 
 * @param buf the path buffer
 * @param N the size of buffer
 * @return the number of pathes found < 0 on error
 **/
int tag_tacker_get_path(uint32_t tag_id, uint32_t sink_closure, tag_tracker_path_t** buf, size_t N);
#endif
