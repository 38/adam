#include <adam.h>
int main()
{
    adam_init();
    dalvik_loader_from_directory("../testdata/AndroidAntlr");
    const char* classname = stringpool_query("antlr/ANTLRLexer");
    const char* methodname = stringpool_query("<clinit>");
    dalvik_type_t* arglist[] = {NULL};
    dalvik_block_from_method(classname, methodname, arglist);
    adam_finalize();
    return 0;
}
