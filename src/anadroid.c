#include <anadroid.h>
void anadroid_init(void)
{
    stringpool_init(STRING_POOL_SIZE);
    dalvik_tokens_init();
}
void anadroid_finalize(void)
{
    stringpool_fianlize();
}
