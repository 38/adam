#include <dalvik/dalvik_field.h>
#include <anadroid.h>
#include <assert.h>

sexpression_t* sexp;
dalvik_field_t* field;

int main()
{
    anadroid_init();

    assert(NULL != (sexp_parse("(field (attrs public static ) test [array [object java/lang/String]])", &sexp)));
    assert(NULL != (field = dalvik_field_from_sexp(sexp, NULL, NULL)));
    dalvik_field_free(field);
    sexp_free(sexp);

    assert(NULL != (sexp_parse("(field (attrs public static ) test [array [object java/lang/String]])", &sexp)));
    assert(NULL != (field = dalvik_field_from_sexp(sexp, NULL, NULL)));
    dalvik_field_free(field);
    sexp_free(sexp);

    anadroid_finalize();
    return 0;
}
