#include <anadroid.h>
#include <cesk/cesk_object.h>
int main()
{
    anadroid_init();
    dalvik_loader_from_directory("../testdata/AndroidAntlr");
    anadroid_finalize();
    return 0;
}
