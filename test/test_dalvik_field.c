#include <dalvik/dalvik_field.h>
#include <adam.h>
#include <assert.h>

sexpression_t* sexp;
dalvik_field_t* field;

int main()
{
	adam_init();

	assert(NULL != (sexp_parse("(field (attrs public static ) test [array [object java/lang/String]])", &sexp)));
	assert(NULL != (field = dalvik_field_from_sexp(sexp, NULL, NULL)));
	dalvik_field_free(field);
	sexp_free(sexp);

	assert(NULL != (sexp_parse("(field (attrs public static ) test [array [object java/lang/String]])", &sexp)));
	assert(NULL != (field = dalvik_field_from_sexp(sexp, NULL, NULL)));
	dalvik_field_free(field);
	sexp_free(sexp);

	adam_finalize();
	return 0;
}
