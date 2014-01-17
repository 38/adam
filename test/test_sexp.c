#include <dalvik/sexp.h>
#include <assert.h>

int main()
{
    stringpool_init(1027);
    sexp_init();
    sexpression_t* exp;
    assert(0 == strcmp("", sexp_parse("move", &exp)));
    assert(exp->type == SEXP_TYPE_LIT);
    assert(*(const char**)exp->data == SEXPR_KW_MOVE);
    //TODO: test the match
    const char* this;
    assert(1 == sexp_match(exp, "L?", &this));
    assert(1 == sexp_match(exp, "L=", SEXPR_KW_MOVE));
    sexp_free(exp);
    assert(0 == strcmp(" remaining", sexp_parse("(move/from16 v123,v456) remaining", &exp)));
    const char* a,*b,*c; 
    sexp_free(exp);
    assert(1 == sexp_match(exp, "(L=L?L?L?", SEXPR_KW_MOVE, &a, &b, &c));
    assert(a == SEXPR_KW_FROM16);
    assert(0 == strcmp(b, "v123"));
    assert(0 == strcmp(c, "v456"));
    sexpression_t* s = exp;
    static const char* excepted[] = {
        "move",
        "from16",
        "v123",
        "v456", 
        NULL
    };
    int i;
    for(i = 0;s;i ++)
    {
        const char* a;
        assert(1 == sexp_match(s, "(L?A", &a, &s));
        assert(0 == strcmp(excepted[i], a));
    }
    assert(excepted[i] == NULL);
    
    return 0;
}
