#ifndef __DALVIK_TYPE_H__
#define __DALVIK_TYPE_H__
/** @file dalvik_type.h
 *  @brief type descriptor
 */
#include <constants.h>

#include <sexp.h>
struct _dalvik_type_t;
/** @brief object data */
typedef struct {
	const char* path;
} dalvik_type_object_t;
/** @brief array data */
typedef struct {
	struct _dalvik_type_t* elem_type;
} dalvik_type_array_t;
/** @brief type code */
enum {
	DALVIK_TYPECODE_VOID  ,
	DALVIK_TYPECODE_INT   ,
	DALVIK_TYPECODE_LONG  ,
	DALVIK_TYPECODE_SHORT ,
	DALVIK_TYPECODE_WIDE  ,
	DALVIK_TYPECODE_FLOAT ,
	DALVIK_TYPECODE_DOUBLE,
	DALVIK_TYPECODE_CHAR,
	DALVIK_TYPECODE_BYTE,
	DALVIK_TYPECODE_BOOLEAN,

	DALVIK_TYPECODE_NUM_ATOM,

	DALVIK_TYPECODE_OBJECT= 0x80,
	DALVIK_TYPECODE_ARRAY = 0x81
};

extern const char* dalvik_type_atom_name[];
/** @brief check if the typecode is atmoic */
#define DALVIK_TYPE_IS_ATOM(typenum) ((~typenum)&0x80)
/** @brief the type descriptor */
typedef struct _dalvik_type_t{
	uint32_t typecode;  /*!< type code */
	union{
		dalvik_type_array_t array;  /*!< the array data */
		dalvik_type_object_t object; /*!< the object data */
	} data;  /*!< data payload */
} dalvik_type_t;

extern dalvik_type_t* dalvik_type_atom[DALVIK_TYPECODE_NUM_ATOM];

/** @brief Initialize this module */
void dalvik_type_init(void);

/** @brief Finalize this module */ 
void dalvik_type_finalize(void);

/** @brief get a dalvik type from a sexpression */
dalvik_type_t* dalvik_type_from_sexp(const sexpression_t* sexp);

/** @brief clone a dalvik type from an existing one */
dalvik_type_t* dalvik_type_clone(const dalvik_type_t* type);

/** @brief clone a dalvik type list from an exisiting one */
const dalvik_type_t**  dalvik_type_list_clone(const dalvik_type_t * const * type);

/** @brief free the memory */
void dalvik_type_free(dalvik_type_t* type);

/** @brief free a NULL-terminating type list */
void dalvik_type_list_free(dalvik_type_t **list);

/** @brief compare if two types are equal */
int dalvik_type_equal(const dalvik_type_t* left, const dalvik_type_t* right);

/**@brief compute a hash code for this type */
hashval_t dalvik_type_hashcode(const dalvik_type_t* type);

/** @brief compute a hash code for a type list */
hashval_t dalvik_type_list_hashcode(const dalvik_type_t * const * typelist);

/** @brief return a bool indicate if type list left and right are equal */
int dalvik_type_list_equal(const dalvik_type_t * const * left, const dalvik_type_t * const * right);

/** @brief convert a type to human readable string */
const char* dalvik_type_to_string(const dalvik_type_t* type, char* buf, size_t sz);

/** @brief convert a type list to human readable string */
const char* dalvik_type_list_to_string(const dalvik_type_t * const * list, char* buf, size_t sz);

#define DALVIK_TYPE_ATOM(what)  dalvik_type_atom[DALVIK_TYPECODE_##what]

#define DALVIK_TYPE_INT DALVIK_TYPE_ATOM(INT)
#define DALVIK_TYPE_LONG DALVIK_TYPE_ATOM(LONG)
#define DALVIK_TYPE_SHORT DALVIK_TYPE_ATOM(SHORT)
#define DALVIK_TYPE_WIDE DALVIK_TYPE_ATOM(WIDE)
#define DALVIK_TYPE_FLOAT DALVIK_TYPE_ATOM(FLOAT)
#define DALVIK_TYPE_DOUBLE DALVIK_TYPE_ATOM(DOUBLE)
#define DALVIK_TYPE_CHAR DALVIK_TYPE_ATOM(CHAR)
#define DALVIK_TYPE_BYTE DALVIK_TYPE_ATOM(BYTE)
#define DALVIK_TYPE_BOOLEAN DALVIK_TYPE_ATOM(BOOLEAN)
#define DALVIK_TYPE_VOID DALVIK_TYPE_ATOM(VOID)

#endif /* __DALVIK_TYPE_H__ */
