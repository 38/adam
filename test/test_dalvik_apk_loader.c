#include <adam.h>
#include <dalvik/dalvik_loader.h>
void load_apk_dir(const char* path)
{
    dalvik_loader_from_directory(path);
}
int main(int argv, char** argc)
{
    adam_init();
    load_apk_dir(argc[1]);
    adam_finalize();
    return 0;
}
