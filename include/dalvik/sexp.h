#ifndef __SEXP_H__
#include <stdint.h>
enum{
    SEXP_NIL,
    SEXP_STR,
    SEXP_LIT,
    SEXP_CONS
};

typedef struct {
    int type;
    char data[0];
} sexpression_t;

typedef struct sexp_nil_t;

typedef const char* sexp_str_t;

typedef const char* sexp_lit_t;

typedef struct{
    sexpression_t * fisrt;
    sexpression_t * second;
} sexp_cons_t;

const char* sexp_parse(const char* str, sexpression_t* buf);

const char* sexp_free(sexpression_t* buf);

#endif /* __SEXP_H__ */
