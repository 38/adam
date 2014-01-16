#ifndef __SEXP_H__
#include <stdint.h>
#include <stdlib.h>

/* Types of S-Expression */ 
enum{
    /* 
     * The reason why we don't need a type for empty S-Expression {} 
     * is it's actully a singleton, we can use NULL pointer to repr it.
     */
    SEXP_TYPE_LIT,    /* A literal */
    SEXP_TYPE_STR,    /* A string */
    SEXP_TYPE_CONS    /* A pair */
};

/* header structure of a S-Expression */
typedef struct {
    int type;
    char data[0];
} sexpression_t;


typedef const char* sexp_str_t;

typedef const char* sexp_lit_t;

typedef struct{
    sexpression_t * first;
    sexpression_t * second;
} sexp_cons_t;

/* Parse a string into sexpression. 
 * str: String to parse
 * buf: the output buffer 
 * return value: The remaining string after current S-Expression has been parsed
 *               NULL indicates an error
 */
const char* sexp_parse(const char* str, sexpression_t** buf);

/* free memory for a S-Expression recursively */
void sexp_free(sexpression_t* buf);

int sexp_init(void);

/* 
 * Check S-Expression matches a pattern 
 * Like printf function, pattern only describe the property of following function
 * The pattern can be defined as
 * PATTERN := ({TYPE DESC}* | TYPE DESC
 * TYPE    :=  C|L|S|A
 * DESC    :=  ?|=|*
 * If the pattern starts with a (, that means the pattern is going to match into a list
 * The type name is either C, L, S, A, which means Cons, Literal and String and Anything
 * The desc field indicates how to use this variable. If the desc is =, that means this
 *     part of the list must have THE SAME TARGET ADDRESS and same type.
 *     Notice that type C can not have '='
 *     '?' means we what to store the value in a variable and also, verify the type
 *     * also require the function store the value, buf * means the program will produce 
 *     an array which contains all values 
 */
int sexp_match(const sexpression_t* sexpr, const char* pattern, ...);

#define SEXP_NIL NULL

#define SEXPR_MAX_NUM_KEYWORDS 256

extern const char* sexpr_keywords[SEXPR_MAX_NUM_KEYWORDS];

#define SEXPR_KW_MOVE       (sexpr_keywords[0])
#define SEXPR_KW_RETURN     (sexpr_keywords[1])
#define SEXPR_KW_CONST      (sexpr_keywords[2])
#define SEXPR_KW_MONITOR    (sexpr_keywords[3])
#define SEXPR_KW_CHECK      (sexpr_keywords[4])
#define SEXPR_KW_INSTANCE   (sexpr_keywords[5])
#define SEXPR_KW_ARRAY      (sexpr_keywords[6])
#define SEXPR_KW_NEW        (sexpr_keywords[7])
#define SEXPR_KW_FILLED     (sexpr_keywords[8])
#define SEXPR_KW_16         (sexpr_keywords[9])
#define SEXPR_KW_FROM16     (sexpr_keywords[10])
#define SEXPR_KW_WIDE       (sexpr_keywords[11])
#define SEXPR_KW_OBJECT     (sexpr_keywords[12])
#define SEXPR_KW_RESULT     (sexpr_keywords[13])
#define SEXPR_KW_EXCEPTION  (sexpr_keywords[14])
#define SEXPR_KW_VOID       (sexpr_keywords[15])
#define SEXPR_KW_4          (sexpr_keywords[16])
#define SEXPR_KW_HIGH16     (sexpr_keywords[17])
#define SEXPR_KW_32         (sexpr_keywords[18])
#define SEXPR_KW_CLASS      (sexpr_keywords[19])
#define SEXPR_KW_JUMBO      (sexpr_keywords[20])
#define SEXPR_KW_ENTER      (sexpr_keywords[21])
#define SEXPR_KW_EXIT       (sexpr_keywords[22])
#define SEXPR_KW_CAST       (sexpr_keywords[23])
#define SEXPR_KW_OF         (sexpr_keywords[24])
#define SEXPR_KW_LENGTH     (sexpr_keywords[25])
#define SEXPR_KW_RANGE      (sexpr_keywords[26])
#define SEXPR_KW_THROW      (sexpr_keywords[27])
#define SEXPR_KW_GOTO       (sexpr_keywords[28])
#define SEXPR_KW_PACKED     (sexpr_keywords[29])
#define SEXPR_KW_SWITCH     (sexpr_keywords[30])
#define SEXPR_KW_SPARSE     (sexpr_keywords[31])
#define SEXPR_KW_CMPL       (sexpr_keywords[32])
#define SEXPR_KW_CMPG       (sexpr_keywords[33])
#define SEXPR_KW_CMP        (sexpr_keywords[34])
#define SEXPR_KW_FLOAT      (sexpr_keywords[35])
#define SEXPR_KW_DOUBLE     (sexpr_keywords[36])
#define SEXPR_KW_LONG       (sexpr_keywords[37])
#define SEXPR_KW_IF         (sexpr_keywords[38])
#define SEXPR_KW_EQ         (sexpr_keywords[39])
#define SEXPR_KW_NE         (sexpr_keywords[40])
#define SEXPR_KW_LE         (sexpr_keywords[41])
#define SEXPR_KW_GE         (sexpr_keywords[42])
#define SEXPR_KW_GT         (sexpr_keywords[43])
#define SEXPR_KW_LT         (sexpr_keywords[44])
#define SEXPR_KW_EQZ        (sexpr_keywords[45])
#define SEXPR_KW_NEZ        (sexpr_keywords[46])
#define SEXPR_KW_LEZ        (sexpr_keywords[47])
#define SEXPR_KW_GEZ        (sexpr_keywords[48])
#define SEXPR_KW_GTZ        (sexpr_keywords[49])
#define SEXPR_KW_LTZ        (sexpr_keywords[50])
#define SEXPR_KW_BOOLEAN    (sexpr_keywords[51])
#define SEXPR_KW_BYTE       (sexpr_keywords[52])
#define SEXPR_KW_CHAR       (sexpr_keywords[53])
#define SEXPR_KW_SHORT      (sexpr_keywords[54])
#define SEXPR_KW_AGET       (sexpr_keywords[55])
#define SEXPR_KW_APUT       (sexpr_keywords[56])
#define SEXPR_KW_SGET       (sexpr_keywords[57])
#define SEXPR_KW_SPUT       (sexpr_keywords[58])
#define SEXPR_KW_IGET       (sexpr_keywords[59])
#define SEXPR_KW_IPUT       (sexpr_keywords[60])
#define SEXPR_KW_INVOKE     (sexpr_keywords[61])
#define SEXPR_KW_VIRTUAL    (sexpr_keywords[62])
#define SEXPR_KW_SUPER      (sexpr_keywords[63])
#define SEXPR_KW_DIRECT     (sexpr_keywords[64])
#define SEXPR_KW_STATIC     (sexpr_keywords[65])
#define SEXPR_KW_INTERFACE  (sexpr_keywords[66])
#define SEXPR_KW_INT        (sexpr_keywords[67])
#define SEXPR_KW_TO         (sexpr_keywords[68])
#define SEXPR_KW_NEG        (sexpr_keywords[69])
#define SEXPR_KW_NOT        (sexpr_keywords[70])
#define SEXPR_KW_ADD        (sexpr_keywords[71])
#define SEXPR_KW_SUB        (sexpr_keywords[72])
#define SEXPR_KW_MUL        (sexpr_keywords[73])
#define SEXPR_KW_DIV        (sexpr_keywords[74])
#define SEXPR_KW_REM        (sexpr_keywords[75])
#define SEXPR_KW_AND        (sexpr_keywords[76])
#define SEXPR_KW_OR         (sexpr_keywords[77])
#define SEXPR_KW_XOR        (sexpr_keywords[78])
#define SEXPR_KW_SHL        (sexpr_keywords[79])
#define SEXPR_KW_SHR        (sexpr_keywords[80])
#define SEXPR_KW_USHL       (sexpr_keywords[81])
#define SEXPR_KW_USHR       (sexpr_keywords[82])
#define SEXPR_KW_2ADDR      (sexpr_keywords[83])
#define SEXPR_KW_LIT8       (sexpr_keywords[84])
#define SEXPR_KW_LIT16      (sexpr_keywords[85])
#endif /* __SEXP_H__ */
