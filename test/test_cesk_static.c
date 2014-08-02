#include <assert.h>
#include <adam.h>

int main()
{
    adam_init();
    /* we need to load a package first */
    assert(dalvik_loader_from_directory("test/data/AndroidAntlr") >= 0);

    adam_finalize();
    return 0;
}
