#include <cesk/cesk.h>
void cesk_init(void)
{
    cesk_value_init();
    cesk_set_init();
}
void cesk_finalize(void)
{
    cesk_set_finalize();
    cesk_value_finalize();
}
