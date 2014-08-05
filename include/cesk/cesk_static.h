/**
 * @file cesk_static.h
 * @brief the static field table & default value parser
 * @details In the store address space, there's an special 
 **/
#ifndef __CESK_STATIC_H__
#define __CESK_STATIC_H__
#include <log.h>
#include <sexp.h>
#include <constants.h>
#include <const_assertion.h>

/**
 * @brief the static field table
 **/
typedef struct _cesk_static_table_t cesk_static_table_t;

/**
 * @brief the iterator type for the static table
 **/
typedef struct _cesk_static_table_iter_t cesk_static_table_iter_t;

/**
 * @biref the iterator to traverse all fields that contains relocated address
 **/
typedef struct _cesk_static_table_reloc_iter_t cesk_static_table_reloc_iter_t;

#include <cesk/cesk_set.h>

/**
 * @brief the static field table
 **/
struct _cesk_static_table_t;

/**
 * @brief the implemenation of the static filed iterator
 **/
struct _cesk_static_table_iter_t {
	const cesk_static_table_t* table;
	uint32_t begin;
};
/**
 * @brief the implementation of the reloc iter
 **/
struct _cesk_static_table_reloc_iter_t {
	cesk_static_table_t* table;
	uint32_t begin;
};
/**
 * @brief initialize this module
 * @return the result for initialization, < 0 indicates an error occured 
 **/
int cesk_static_init();
/**
 * @brief finalize this module
 * @return nothing
 **/
void cesk_static_finalize();
/**
 * @brief look for the static field in the member dict and translate the default value to cesk value
 * @param class the name of the target class
 * @param field the field name of the target
 * @return the field address, CESK_STORE_ADDR_NULL indicates errors 
 **/
uint32_t cesk_static_field_query(const char* class, const char* field);
/**
 * @brief make a copy for given static field table
 * @param table the source table, if the table is NULL we just create an empty static table
 *        in which everything remains uninitialized
 * @return the pointer to the newly created static table
 **/
cesk_static_table_t* cesk_static_table_fork(const cesk_static_table_t* table);
/**
 * @brief free a static field table
 * @param table
 * @return nothing
 **/
void cesk_static_table_free(cesk_static_table_t* table);
/**
 * @brief get the value of a given static address 
 * @param table the static table
 * @param addr the static field address
 * @param init if this value is zero, means if we find a unitialized field, we just return empty rather than initialize it
 * @note because this function is resonsible for field initialization, so that we need a write permission
 * @return the result set, NULL indicates errors
 **/
const cesk_set_t* cesk_static_table_get_ro(const cesk_static_table_t* table, uint32_t addr, int init);

/**
 * @brief get a writable pointer to the slot that contains the value for the given static field
 * @param table the pointer to the table reference to the static table(the table address might be change after this function)
 * @param addr the address 
 * @param init need initialization?
 * @return the pointer to the slot that contains the static field
 **/
cesk_set_t** cesk_static_table_get_rw(cesk_static_table_t* table, uint32_t addr, int init);

/**
 * @brief release a writable pointer to the slot that contains the value for the given static field
 * @param table the static table
 * @param addr the address to that static field
 * @param value the reference to new value 
 * @return the result for the operation, < 0 indicates an error
 **/
int cesk_static_table_release_rw(cesk_static_table_t* table, uint32_t addr, const cesk_set_t* value);

/**
 * @brief update the relocated flag of a field
 * @param table the static field table
 * @param addr the target address
 * @param val the flag value
 * @param assume_writable if noreuse = 1, the caller assumes that the the node is currently writable
 * @return the result < 0 indicates an error
 **/
int cesk_static_table_update_relocated_flag(cesk_static_table_t* table, uint32_t addr, uint32_t val, uint32_t assume_writable);

/**
 * @brief find the first static field that contains a relocated address
 * @param table the static field table
 * @return the register reference number, CESK_STORE_ADDR_NULL if there's no field contains relocated address
 **/
uint32_t cesk_static_table_first_reloc(cesk_static_table_t* table);

/**
 * @brief initialize a new iterator to traverse the table
 * @param table the static field table
 * @param iter the memory for the iterator
 * @return the pointer to newly created iterator, NULL indicates error
 **/
cesk_static_table_iter_t* cesk_static_table_iter(const cesk_static_table_t* table, cesk_static_table_iter_t* iter);

/**
 * @brief move the iterator one step forward and return the previous value
 * @param iter the iterator
 * @param paddr the pointer to a uint32_t buffer for the address to this field, NULL indicates
 *              the caller do not need this address
 * @return the value set for this field, NULL indicates there's end of the table
 **/
const cesk_set_t* cesk_static_table_iter_next(cesk_static_table_iter_t* iter, uint32_t *paddr);


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

/**
 * @brief compute the hashcode of the static field table based on the content, for testing purpose only
 * @param table the static field table
 * @return the hashval computed from this table
 * @note this function is used for verify the hashcode field in the table maintaning correctly
 **/
hashval_t cesk_static_table_compute_hashcode(const cesk_static_table_t* table);
/**
 * @brief convert this static table to string
 * @param table the static table to convert
 * @param buf the result buffer
 * @param sz the size of result buffer 
 * @return the pointer the result string, NULL indicates an error 
 **/
const char* cesk_static_table_to_string(const cesk_static_table_t* table, char* buf, size_t sz);
/**
 * @brief print debug infomation in DEBUG log
 * @param table the static field table to print
 * @return nothing
 **/
void cesk_static_table_print_debug(const cesk_static_table_t* table);
#endif
