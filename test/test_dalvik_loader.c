#include <adam.h>
#include <dalvik/dalvik_loader.h>
#include <assert.h>

int main()
{
	adam_init();
	assert(0 == dalvik_loader_from_directory("test/data"));
	dalvik_loader_summary();
	adam_finalize();
	return 0;
}
