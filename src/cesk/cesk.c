#include <cesk/cesk.h>
void cesk_init(void)
{
    cesk_value_init();
    cesk_set_init();
	cesk_diff_init();
}
void cesk_finalize(void)
{
	cesk_diff_finalize();
    cesk_set_finalize();
    cesk_value_finalize();
}
