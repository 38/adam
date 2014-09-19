/**
 * @brief tracker the value tags
 * @file tag_tracker.h
 **/
#ifndef __TAG_TRACKER_H__
#define __TAG_TRACKER_H__
#include <tag/tag_set.h>
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
 * @brief open a instruction executing transaction
 * @param closure current closure
 * @param instruction current instruction index
 * @return < 0 on error
 **/
int tag_tracker_instruction_transaction_begin(uint32_t closure, uint32_t instruction);
/**
 * @brief open a block merging transaction
 * @param closure the current closure index
 * @param blockidx the current block index
 * @return < 0 on error
 **/
int tag_tracker_block_transaction_begin(uint32_t closure, uint32_t blockidx);
/**
 * @brief open a branch initlaization transaction
 * @param closure the current closure index
 * @param from the source block
 * @param to the destination block
 * @return < 0 on error
 **/
int tag_tracker_branch_transaction_begin(uint32_t closure, uint32_t from, uint32_t to);
/**
 * @brief close a transaction
 * @return < 0 on error
 **/
int tag_tracker_transaction_close();
/**
 * @brief register a new tagset in tracker
 * @param tsid the tagset id
 * @param tagset the newly created tagset
 * @param inputs array of input tagset index
 * @param ninputs how many inputs
 * @return < 0 on error
 **/
int tag_tracker_register_tagset(uint32_t tsid, const tag_set_t* tagset, const uint32_t* inputs, uint32_t ninputs);

/**
 * @brief get data-flow path
 * @param tag_id the tag id to focus on
 * @param tagset_id the id of tagset that we are instrested in 
 * @param buf the path buffer
 * @param N the size of buffer
 * @return the number of pathes found < 0 on error
 **/
int tag_tacker_get_path(uint32_t tag_id, uint32_t tagset_id, uint32_t** instruction, size_t N);
#endif
