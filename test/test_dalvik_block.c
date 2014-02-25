#include <adam.h>
#include <assert.h>
int main()
{
    adam_init();
    dalvik_loader_from_directory("test/data/AndroidAntlr");
    sexpression_t* sexp;
    sexp_parse("[object java/lang/String]", &sexp);
    dalvik_type_t *type = dalvik_type_from_sexp(sexp);
    sexp_free(sexp);

    const char* classname = stringpool_query("antlr/ANTLRParser");
    const char* methodname = stringpool_query("treeParserSpec");
    dalvik_type_t* arglist[] = {NULL ,NULL};
    arglist[0] = type;
    dalvik_block_t* block = dalvik_block_from_method(classname, methodname, (const dalvik_type_t**)arglist);
    dalvik_type_free(type);
    assert(NULL != block);
    adam_finalize();
    return 0;
}
