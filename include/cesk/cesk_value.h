#ifndef __CESK_VALUE_T__
#define __CESK_VALUE_T__
#include <constants.h>

/**
 * @file cesk_value.h
 * @brief Abstract value of Davlik CESK Virtual Machine
 *
 * @details
 * The process of looking for a value :
 *
 *             Mem Address         Offest                   RegisterName
 * Global Value <----- Frame Store <----- Register/Field Set <------ Frame 
 *
 * This file constains defination of Global Values
 *
 */

/* previous defination */
typedef  struct _cesk_value_t cesk_value_t;
typedef  struct _cesk_value_const_t cesk_value_const_t;

#include <dalvik/dalvik_instruction.h>
#include <cesk/cesk_object.h>
#include <cesk/cesk_set.h>

/** @brief type code of the value */
enum{
	CESK_TYPE_OBJECT,	/*!<object type*/
#if 0
	CESK_TYPE_ARRAY,	/*!<array type*/
#endif /* We do not support array type any more, because we can use built-in object to represent it  */
	CESK_TYPE_SET		/*!<set type*/
};

/** @brief this macro this used to generate the member definition 
 * 		   of pointer list. Because for the const value and non-const
 * 		   value, the member name should be the same. That is why we 
 * 		   should use this approach to do this
 */
#define __CESK_POINTER_LIST(prefix) \
		prefix void*	     _void;	/*!<as a void pointer */\
		prefix cesk_set_t*    set;   /*!<as a set */\
		prefix cesk_object_t* object; /*!<as an object */

/** 
 * @brief the data structure for a abstruct value 
 * @todo  maintain the reloc bit
 **/
struct _cesk_value_t {
	uint8_t     type:6;         /*!<type of this value */
	uint8_t     reloc:1;        /*!<does the object contains an relocation address? */
	uint8_t     write_status:1; /*!<this bit check if the value is associated with a writable pointer */
	union {
		__CESK_POINTER_LIST()
	} pointer;              /*!<actuall data pointer*/
	uint32_t    refcnt;     /*!<the reference counter */
	hashval_t   hashcode;   /*!<the hashcode */
	struct _cesk_value_t  *prev, 
						  *next; /*!<the previous and next pointer used by value list */
}; 

/** @brief the abstruct value that can not be modified */
struct _cesk_value_const_t {
	const uint8_t     type:7;       /*!<type of this value */
	const uint8_t     write_status:1; /*!<this bit check if the value is associated with a writable pointer */
	const union {
		__CESK_POINTER_LIST(const)
	} pointer; /*!<actuall data pointer*/
};

#undef __CESK_POINTER_LIST

/** @brief initilize 
 *  @return < 0 for error
 */
int cesk_value_init();
/** @brief finalize 
 *  @return nothing
 */
void cesk_value_finalize();

/** 
 * @brief create a new value using the class 
 * @param classpath the class path of the class
 * @return the value contains new object
 **/
cesk_value_t* cesk_value_from_classpath(const char* classpath);
/** 
 * @brief create an empty-set value 
 * @return the new empty set
 **/
cesk_value_t* cesk_value_empty_set();
/** 
 * @brief fork the value, inorder to modify 
 * @return the copy of the store
 **/
cesk_value_t* cesk_value_fork(const cesk_value_t* value);
/** 
 * @brief increase the reference counter 
 * @return nothing
 **/
void cesk_value_incref(cesk_value_t* value);
/** 
 * @brief decrease the reference counter 
 **/
void cesk_value_decref(cesk_value_t* value);

/** 
 * @brief The address based hashcode. Obviously, if every cell in a store are equal to the conresponding cell in
 * another store, the frame is equal obviously. However, there are some cases, e.g. allocate the same value
 * by different instruction ( this leading a different address ), the comparse function returns false, but
 * the store is actually the same.
 * But because the number of address is finite, the function will terminate using this method.
 * @return hash code
 */
hashval_t cesk_value_hashcode(const cesk_value_t* value);
/**
 * @brief non-inremental style hash code
 * @return hash code
 **/
hashval_t cesk_value_compute_hashcode(const cesk_value_t* value);

/**
 * @brief compare, the compare fuction is also based on the addr compare 
 * @return 1 for two value are equal
 **/
int cesk_value_equal(const cesk_value_t* first, const cesk_value_t* second);

/**
 * @brief set the reloc bit
 * @param value
 * @return nothing
 **/
static inline void cesk_value_set_reloc(cesk_value_t* value)
{
	value->reloc = 1;
}
/**
 * @brief get the reloc bit
 * @param value
 * @return the value of reloc bit
 **/
static inline int cesk_value_get_reloc(cesk_value_t* value)
{
	return value->reloc;
}
/**
 * @brief clear the reloc bit
 * @param value
 * @return nothing
 **/
static inline void cesk_value_clear_reloc(cesk_value_t* value)
{
	value->reloc = 0;
}
#endif /* __CESK_VALUE_T__ */
