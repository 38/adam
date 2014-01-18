#include <anadroid.h>
void anadroid_init(void)
{
    stringpool_init(STRING_POOL_SIZE);
    dalvik_tokens_init();
    dalvik_label_init(); 
}
void anadroid_finalize(void)
{
    dalvik_label_finalize();
    stringpool_fianlize();
}
