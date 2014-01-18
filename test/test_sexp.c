#include <sexp.h>
#include <dalvik/dalvik_tokens.h>
#include <assert.h>

int main()
{
    stringpool_init(1027);
    dalvik_tokens_init();
    sexpression_t* exp;
    assert(0 == strcmp("", sexp_parse("move", &exp)));
    assert(exp->type == SEXP_TYPE_LIT);
    assert(*(const char**)exp->data == DALVIK_TOKEN_MOVE);
    //TODO: test the match
    const char* this;
    assert(1 == sexp_match(exp, "L?", &this));
    assert(1 == sexp_match(exp, "L=", DALVIK_TOKEN_MOVE));
    sexp_free(exp);
    assert(0 == strcmp(" remaining", sexp_parse("(move/from16 v123,v456) remaining", &exp)));
    const char* a,*b,*c; 
    assert(1 == sexp_match(exp, "(L=L?L?L?", DALVIK_TOKEN_MOVE, &a, &b, &c));
    assert(a == DALVIK_TOKEN_FROM16);
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
    sexp_free(exp);
    // Strip test;
    sexpression_t* stripped;
    assert(0 == strcmp("", sexp_parse("(move v123,v456)", &exp)));
    assert(NULL != (stripped = sexp_strip(exp, DALVIK_TOKEN_MOVE, DALVIK_TOKEN_SHR, NULL)));
    assert(stripped != exp);
    assert(sexp_match(stripped ,"(L=L=", stringpool_query("v123"), stringpool_query("v456")));
    
    assert(NULL != (stripped = sexp_strip(exp, DALVIK_TOKEN_MONITOR, DALVIK_TOKEN_SHR, NULL)));
    assert(stripped == exp);
    assert(sexp_match(stripped ,"(L=L=L=", DALVIK_TOKEN_MOVE, stringpool_query("v123"), stringpool_query("v456")));
    sexp_free(exp);

    assert(0 == strcmp("", sexp_parse("(java/utils/xxxxx)", &exp)));
    assert(0 == strcmp("java/utils/xxxxx",sexp_get_object_path(exp)));
    sexp_free(exp);
    return 0;
}
