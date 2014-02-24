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
struct   _cesk_value_t;
typedef  struct _cesk_value_t cesk_value_t;

#include <dalvik/dalvik_instruction.h>
#include <cesk/cesk_object.h>
#include <cesk/cesk_set.h>

/** @brief type code of the value */
enum{
#if 0
    CESK_TYPE_NUMERIC,   /* we treat char as a numeric value */
    CESK_TYPE_BOOLEAN,
#endif
    CESK_TYPE_OBJECT,	/*!<object type*/
    CESK_TYPE_ARRAY,	/*!<array type*/
    CESK_TYPE_SET		/*!<set type*/
};

#if 0
#define CESK_VALUE_NUMERIC_POSITIVE 1  /* if a numeric value is positive */
#define CESK_VALUE_NUMERIC_ZERO 0       /* if a numeric value is zero */
#define CESK_VALUE_NUMERIC_NEGATIVE -1 /* if a numeric value is negative */
typedef int8_t cesk_value_numeric_t;    /* a numberic value */

/* abstract boolean */
#define CESK_VALUE_BOOLEAN_TRUE  1
#define CESK_VALUE_BOOLEAN_FALSE 0
typedef int8_t cesk_value_boolean_t;

/* abstract string */
typedef int8_t cesk_value_string_t[0];
#endif 

/* abstract object */
#include <cesk/cesk_object.h>

/** @brief abstract array */
typedef struct {
    cesk_set_t* values;  /*!<the value set of the array */
} cesk_value_array_t;

/* set */
#include <cesk/cesk_set.h>

/** @brief the data structure for a abstruct value */
typedef struct _cesk_value_t {
    uint8_t     type:7;       /*!<type of this value */
    uint8_t     write_status:1; /*!<this bit check if the value is associated with a writable pointer */
    uint32_t    refcnt;     /*!<the reference counter */
    hashval_t   hashcode;   /*!<the hashcode */
    struct _cesk_value_t 
                *prev, *next; /*!<the previous and next pointer used by value list */
    char        data[0];    /*!<actual data payload */
} cesk_value_t; 


/* 
 * What initilize function does :
 * 1. for numeric / string / boolean value, create all possible value
 *    first
 */
/** @brief initilize 
 *  @return nothing
 */
void cesk_value_init();
/** @brief finalize 
 *  @return nothing
 */
void cesk_value_finalize();

#if 0
/* We calculate the hash function and cache it in frame store */
hashval_t cesk_value_hashcode(cesk_value_t* value);
#endif 
/** @brief create a new value using the class 
 *  @param classpath the class path of the class
 *  @return the value contains new object
 */
cesk_value_t* cesk_value_from_classpath(const char* classpath);
#if 0
/* because we use special store address to represent the 
 * values, so, we don't need the singleton for atom values
 * any more 
 */
/* create a new value from a constant operand */
cesk_value_t* cesk_value_from_operand(dalvik_operand_t* operand);
#endif
/** @brief create an empty-set value 
 *  @return the new empty set
 */
cesk_value_t* cesk_value_empty_set();
/** @brief fork the value, inorder to modify 
 *  @return the copy of the store
 */
cesk_value_t* cesk_value_fork(cesk_value_t* value);
/** @brief increase the reference counter 
 *	@return nothing
 * */
void cesk_value_incref(cesk_value_t* value);
/** @brief decrease the reference counter */
void cesk_value_decref(cesk_value_t* value);

/** @brief The address based hashcode. Obviously, if every cell in a same are equal to the conresponding cell in
 * Another store, the frame is equal acutally. However, there are some cases, e.g. allocate the same value
 * by different instruction ( this leading a different address ), the comparse function returns false, but
 * the store is actually the same.
 *
 * But because the number of address is finite, the function will terminate using this method.
 *
 * @return hash code
 */
hashval_t cesk_value_hashcode(cesk_value_t* value);
/** @brief non-inremental style hash code
 *  @return hash code
 */
hashval_t cesk_value_compute_hashcode(cesk_value_t* value);

/* @brief compare, the compare fuction is also based on the addr compare 
 * @return 1 for two value are equal
 */
int cesk_value_equal(cesk_value_t* first, cesk_value_t* second);

#endif /* __CESK_VALUE_T__ */
