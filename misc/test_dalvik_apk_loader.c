#include <adam.h>
#include <dalvik/dalvik_loader.h>
void load_apk_dir(const char* path)
{
    dalvik_loader_from_directory(path);
}
int main(int argv, char** argc)
{
    adam_init();

	if(argv > 1)
    	load_apk_dir(argc[1]);
	else
		fprintf(stderr, "load a package, usage: %s pakgedir", argc[0]);
    adam_finalize();
    return 0;
}
