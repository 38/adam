#ifndef __CESK_VALUE_T__
#define __CESK_VALUE_T__
#include <constants.h>

/* previous defination */
struct   _cesk_value_t;
typedef  struct _cesk_value_t cesk_value_t;
struct   _cesk_value_set_t;
typedef  struct _cesk_value_set_t cesk_value_set_t;

#include <dalvik/dalvik_instruction.h>
#include <cesk/cesk_object.h>

/* Abstract Value */
enum{
    CESK_TYPE_NUMERIC,   /* we treat char as a numeric value */
    CESK_TYPE_STRING,
    CESK_TYPE_BOOLEAN,
    CESK_TYPE_OBJECT,
    CESK_TYPE_ARRAY
};

/* abstract numeric value */
#define CESK_VALUE_NUMERIC_POSITIVE 1  /* if a numeric value is positive */
#define CESK_VALUE_NUMERIC_ZERO 0       /* if a numeric value is zero */
#define CESK_VALUE_NUMERIC_NEGATIVE -1 /* if a numeric value is negative */
typedef int8_t cesk_value_numeric_t;    /* a numberic value */

/* abstract string value */
/* Nothing to abstract, just the type itself */
typedef int8_t cesk_string_value[0];

/* abstract boolean */
#define CESK_VALUE_BOOLEAN_TRUE  1
#define CESK_VALUE_BOOLEAN_FALSE 0
typedef int8_t cesk_value_boolean_t;

/* abstract string */
typedef int8_t cesk_value_string_t[0];

/* abstract object */
#include <cesk/cesk_object.h>

/* abstract array */
typedef struct {
    cesk_value_set_t* values;
} cesk_value_array_t;

typedef struct _cesk_value_t {
    uint8_t     type;      /* type of this value */
    char        data[0];   /* actuall data payload */
} cesk_value_t;   /* this is a abstract value */


typedef struct _cesk_value_set_t cesk_value_set_t;

/* 
 * What initilize function does :
 * 1. for numeric / string / boolean value, create all possible value
 *    first
 */
void cesk_value_init();
void cesk_value_finalize();

hashval_t cesk_value_hashcode(cesk_value_t* value);
hashval_t cesk_value_set_hashcode(cesk_value_set_t* value);

#endif /* __CESK_VALUE_T__ */
