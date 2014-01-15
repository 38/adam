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
    sexpression_t * fisrt;
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

#define SEXP_NIL NULL

#endif /* __SEXP_H__ */
