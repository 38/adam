#include <anadroid.h>
#include <dalvik/dalvik_loader.h>

int main()
{
    anadroid_init();
    dalvik_loader_from_directory("../testdata");
    dalvik_loader_summary();
    anadroid_finalize();
}
