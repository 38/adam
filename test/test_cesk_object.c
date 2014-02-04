#include <anadroid.h>
#include <cesk/cesk_object.h>
int main()
{
    anadroid_init();
    dalvik_loader_from_directory("../testdata/AndroidAntlr");
    dalvik_loader_summary();
    cesk_object_new(stringpool_query("antlr/ANTLRTokdefParser"));
    anadroid_finalize();
    return 0;
}
