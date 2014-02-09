#include <anadroid.h>
#include <assert.h>

int main()
{
    anadroid_init();
    const char* pooled_str;
    int i;
    for(i = 0; i < 10000; i ++)
    {
        char buf[128];
        sprintf(buf, "l%d", i);
        pooled_str = stringpool_query(buf);
        assert(pooled_str != NULL);
        int id;
        id = dalvik_label_get_label_id(pooled_str);
        assert(id >= 0);
        LOG_DEBUG("get label id %d for %s", id, buf);
    }
    for(i = 0; i < 10000;i ++)
    {

        char buf[128];
        sprintf(buf, "l%d", i);
        pooled_str = stringpool_query(buf);
        assert(pooled_str != NULL);
        int id;
        id = dalvik_label_get_label_id(pooled_str);
        assert(id == i);
    }

    anadroid_finalize();
    return 0;
}
