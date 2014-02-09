#include <adam.h>
#include <dalvik/dalvik_loader.h>

int main()
{
    adam_init();
    dalvik_loader_from_directory("../testdata");
    dalvik_loader_summary();
    adam_finalize();
    return 0;
}
