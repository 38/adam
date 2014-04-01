#include <adam.h>
void adam_init(void)
{
	log_init();
	stringpool_init(STRING_POOL_SIZE);
	dalvik_init();
	cesk_init();
}
void adam_finalize(void)
{
	cesk_finalize();
	dalvik_finalize();
	stringpool_fianlize();
	log_finalize();
}
