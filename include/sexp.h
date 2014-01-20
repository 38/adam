#ifndef __SEXP_H__
#define __SEXP_H__
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
    char seperator;
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

/* 
 * Check S-Expression matches a pattern 
 * Like printf function, pattern only describe the property of following function
 * The pattern can be defined as
 * PATTERN := ({TYPE DESC}* | TYPE DESC
 * TYPE    :=  C|L|S|A
 * DESC    :=  ?|=
 * If the pattern starts with a (, that means the pattern is going to match into a list
 * The type name is either C, L, S, A, which means Cons, Literal and String and Anything
 * The desc field indicates how to use this variable. If the desc is =, that means this
 *     part of the list must have THE SAME TARGET ADDRESS and same type.
 *     Notice that type C can not have '='
 *     '?' means we what to store the value in a variable and also, verify the type
 *     The type A is only used at the end of list, which return the remaining list
 */
int sexp_match(const sexpression_t* sexpr, const char* pattern, ...);

/* strip one expected elements (either string or literal) in front of the sexpr if there's some,
 * This is useful because, instructions like
 * move, move/16, move/from16 has no difference.
 * sexpr : the S-Expression to be stripped. 
 * VA    : the expected value, all values are const char*. the arg list ends with a NULL pointer
 * for example sexpr = (from16 v123,456)
 * sexp_strip(sexpr, "from16", "16", NULL) = (v123,456)
 * sexp_strip(sexpr, "abc", "def", NULL) = (from16 v123,456)
 * NOTICE: All string assumed to be a pooled string, that means address equal <==> value equal 
 */
sexpression_t* sexp_strip(sexpression_t* sexpr, ...);

/* Get object path (a b c) ==> a/b/c */
const char* sexp_get_object_path(sexpression_t* sexpr, sexpression_t** remaining);

#define SEXP_NIL NULL

#endif /* __SEXP_H__ */
