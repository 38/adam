/**
 * @file cesk_static.h
 * @brief the static field table & default value parser
 * @detials In the store address space, there's an special 
 **/
#ifndef __CESK_STATIC_H__
#define __CESK_STATIC_H__
#include <log.h>
#include <sexp.h>
#include <constants.h>
#include <const_assertion.h>

typedef struct _cesk_static_table_t cesk_static_table_t;

#include <cesk/cesk_set.h>

/**
 * @brief the static field table
 **/
struct _cesk_static_table_t;

/**
 * @brief create a new empty(which means it's just the initial state) static table
 * @note  because for the table we actually act a lazy initialize policy, which means 
 *        everything remains uninitialized when the table has been created. 
 *        Once some value has been requested by the frame, the program will look for the
 *        value for this field. If not found, just create a new intialize value for this
 *        In addition, this object is also an copy-on-write object. That means the pointer
 *        will be shared by multiple frames, therefore we need a refcount in this table
 * @return the pointer to the newly created static table
 **/
cesk_static_table_t* cesk_static_table_new();
/**
 * @brief `fork' another copy of this table
 * @param table the static table we want to fork
 * @return the pointer to the result static table
 **/
cesk_static_table_t* cesk_static_table_fork(const cesk_static_table_t* table);
/**
 * @brief release the reference to this static table, if the refcnt has been reduced to zero, this object is dead
 * @param table the target static table
 * @return the operation result, < 0 indicates errors
 **/
int cesk_static_table_decref(cesk_static_table_t* table);
/**
 * @brief get the value of a given static address 
 * @param table the static table
 * @return the result set
 **/
const cesk_set_t* cesk_static_table_get_ro(const cesk_static_table_t* table, uint32_t addr);

/**
 * @brief get a writable pointer to the slot that contains the value for the given static field
 * @param table the static table
 * @param addr the address 
 * @return the pointer to the slot that contains the static field
 **/
cesk_set_t** cesk_static_table_get_rw(cesk_static_table_t* table, uint32_t addr);

/**
 * @brief release a writable pointer to the slot that contains the value for the given static field
 * @param table the static table
 * @param addr the address to that static field
 * @return the result for the operation, < 0 indicates an error
 **/
int cesk_static_table_release_rw(cesk_static_table_t* table, uint32_t addr);

/**
 * @brief return the hash code for this static table
 * @param table the static field table
 * @return the result hash code
 **/
hashval_t cesk_static_table_hashcode(const cesk_static_table_t* table);

/**
 * @brief return if or not the two given static field table are the same
 * @param left the first static field table
 * @param right the scond static field table
 * @return the result of comparasion, 1 means two table are equal, 0 means not equal
 *         < 0 indicates can not compare
 **/
int cesk_static_table_equal(const cesk_static_table_t* left, const cesk_static_table_t* right);

#endif
