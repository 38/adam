#include <adam.h>
#include <assert.h>
int main()
{
	dalvik_instruction_init();
	assert(NULL!=dalvik_instruction_new());
	assert(NULL!=dalvik_instruction_pool);
	dalvik_instruction_finalize();
	return 0;
}
