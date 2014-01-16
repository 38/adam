#include <dalvik/sexp.h>
#include <assert.h>

int main()
{
    stringpool_init(1027);
    sexp_init();
    sexpression_t* exp;
    strcmp(" remaining", sexp_parse("(move/from16 v123,v456) remaining", &exp));
    //TODO: test the match
    return 0;
}
