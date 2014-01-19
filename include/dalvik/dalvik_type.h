#ifndef __DALVIK_TYPE_H__
#define __DALVIK_TYPE_H__
#include <sexp.h>
struct _dalvik_type_t;
typedef struct {
    const char* path;
} dalvik_type_object_t;
typedef struct {
    struct _dalvik_type_t* elem_type;
} dalvik_type_array_t;
enum {
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
#define DALVIK_TYPE_IS_ATOM(typenum) ((typenum)&0x80)
typedef struct _dalvik_type_t{
    uint32_t typecode;
    union{
        dalvik_type_array_t array;
        dalvik_type_object_t object;
    } data;
} dalvik_type_t;

extern dalvik_type_t* dalvik_type_atom[DALVIK_TYPECODE_NUM_ATOM];

/* Initialize this module */
void dalvik_type_init(void);

/* Finalize this module */ 
void dalvik_type_finalize(void);

/* get a dalvik type from a sexpression */
dalvik_type_t* dalvik_type_from_sexp(sexpression_t* sexp);

/* free the memory */
void dalvik_type_free(dalvik_type_t* sexp);

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

#endif /* __DALVIK_TYPE_H__ */
