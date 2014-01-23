#include <anadroid.h>
#include <dalvik/dalvik_loader.h>

int main()
{
    anadroid_init();
    dalvik_loader_from_directory("../pushdownoo/jdex2sex/test");
    anadroid_finalize();
}
