#ifndef __DALVIK_INS_H__
#define __DALVIK_INS_H__
#include <stdint.h>

#include <dalvik/sexp.h>

#include <vector.h>

#include <dalvik/label.h>

#ifndef DALVIK_POOL_INIT_SIZE
#   define DALVIK_POOL_INIT_SIZE 1024
#endif

/* Define all opcodes for Dalvik Virtual Machine */
enum {
    DVM_NOP,
    DVM_MOVE,
    DVM_RETURN,
    DVM_CONST,
    DVM_MONITOR,
    DVM_CHECK_CAST,
    DVM_INSTANCE,
    DVM_ARRAY,
    DVM_THROW,
    DVM_GOTO,
    DVM_SWITCH,
    DVM_CMP,
    DVM_IF,
    DVM_INVOKE,
    DVM_UNOP,
    DVM_BINOP,
    DVM_NUM_OF_OPCODE    /* How many items in the enumerate */
};

/* Size specifier */
enum{
    DVM_OPERAND_SIZE_32,
    DVM_OPERAND_SIZE_64
};
#define DVM_OPERAND_FLAG_WIDE       0x1u
/* Type Specifier */
enum {
    DVM_OPERAND_TYPE_ANY,
    DVM_OPERAND_TYPE_BOOLEAN,
    DVM_OPERAND_TYPE_BYTE,
    DVM_OPERAND_TYPE_CHAR,
    DVM_OPERAND_TYPE_SHORT,
    DVM_OPERAND_TYPE_INT,
    DVM_OPERAND_TYPE_LONG,
    DVM_OPERAND_TYPE_DOUBLE,
    DVM_OPERAND_TYPE_FLOAT,
    DVM_OPERAND_TYPE_OBJECT,
    DVM_OPERAND_TYPE_STRING,
    DVM_OPERAND_TYPE_CLASS, 
    DVM_OPERAND_TYPE_VOID,
    DVM_OPERAND_TYPE_LABEL,
    DVM_OPERAND_TYPE_LABELVECTOR
};
#define DVM_OPERAND_FLAG_TYPE(what) ((what)<<1)
#define DVM_OPERAND_FLAG_CONST      0x20
#define DVM_OPERAND_FLAG_RESULT     0x40
#define DVM_OPERAND_FLAG_EXCEPTION  0x80
typedef union {
    uint8_t    flags;
    struct {
        uint8_t size:1;            /* Size of the operand, 32 bit or 64 bit */
        uint8_t type:4;            /* Type specified by the instruction. 
                                      If it's unspecified, it should be DVM_OPERAND_TYPE_ANY */
        uint8_t is_const:1;        /* Wether or not the oprand a constant */
        uint8_t is_result:1;       /* move-result instruction actually take 2 args, but one is a result */
        uint8_t is_expection:1;    /* if the oprand a exception */
    } info;
} dalvik_operand_header;

struct _dalvik_instruction_t;

typedef struct {
    dalvik_operand_header header;
    union{
        const char*        string;  /* for a string constant */
        const char*        methpath; /* for a method path */
        uint8_t            uint8;     
        uint16_t           uint16;
        uint32_t           uint32;
        uint64_t           uint64;
        int8_t             int8;
        int16_t            int16;
        int32_t            int32;
        int64_t            int64;
        double             real64;
        float              real32;
        int32_t            labelid;             /* label id in label pool */
        vector_t*          branches;            /* a group of branch */
    } payload;
} dalvik_operand_t;

typedef struct _dalvik_instruction_t{
    uint8_t            opcode:4;
    uint8_t            num_oprands:2;
    uint16_t        flags:10;
    dalvik_operand_t operands[4];
    const char*     path;
    int             line;
    struct _dalvik_instruction_t* next;
} dalvik_instruction_t;

enum {
    DVM_FLAG_MONITOR_ENT,
    DVM_FLAG_MONITOR_EXT
};

enum {
    DVM_FLAG_INSTANCE_OF,
    DVM_FLAG_INSTANCE_NEW,
    DVM_FLAG_INSTANCE_GET,
    DVM_FLAG_INSTANCE_PUT,
    DVM_FLAG_INSTANCE_SGET,        /* Get a static member */
    DVM_FLAG_INSTANCE_SPUT         /* Put a static member */
};

enum {
    DVM_FLAG_ARRAY_LENGTH,
    DVM_FLAG_ARRAY_NEW,
    DVM_FLAG_ARRAY_FILLED_NEW,
    DVM_FLAG_ARRAY_FILLED_NEW_RANGE,
};

enum {
    DVM_FLAG_INVOKE_VIRTUAL,
    DVM_FLAG_INVOKE_SUPER,
    DVM_FLAG_INVOKE_DIRECT,
    DVM_FLAG_INVOKE_STATIC,
    DVM_FLAG_INVOKE_INTERFACE
};

enum {
    DVM_IF_EQ,
    DVM_IF_NE,
    DVM_IF_GT,
    DVM_IF_GE,
    DVM_IF_LE,
    DVM_IF_LT
};

enum {
    DVM_UOP_NEG,
    DVM_UOP_NOT,
    DVM_UOP_TO,
};

enum {
    DVM_BINOP_ADD,
    DVM_BINOP_SUB,
    DVM_BINOP_MUL,
    DVM_BINOP_DIV,
    DVM_BINOP_REM,
    DVM_BINOP_AND,
    DVM_BINOP_OR,
    DVM_BINOP_SHR,
    DVM_BINOP_SHL,
    DVM_BINOP_USHR,
    DVM_BINOP_USHL
};

extern dalvik_instruction_t* dalvik_instruction_pool;
/* Return a new empty dalvik instruction */
dalvik_instruction_t* dalvik_instruction_new( void );
int dalvik_instruction_init( void );
int dalvik_instruction_finalize( void );

/* 
 * make a new dalvik instruction from a S-Expression
 * sexp: The S-Expression to be convert
 * buf:  Store the result
 * return value: <0 error, 0 success
 */ 
int dalvik_instruction_from_sexp(sexpression_t* sexp, dalvik_instruction_t* buf, int line, const char* file);
void dalvik_instruction_free(dalvik_instruction_t* buf);

/* Error Codes */

#endif /* __DALVIK_INS_H__ */
