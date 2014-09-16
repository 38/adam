#include <bci/bci_interface.h>
#include <stringpool.h>
typedef struct{
	uint32_t init_state;
	cesk_set_t* isr;
} this_t;
static const char* kw_init;
int java_io_BufferedReader_init()
{
	kw_init = stringpool_query("<init>");
	return 0;
}
bci_class_t java_io_BufferedReader_metadata = {
	.size = sizeof(this_t),
	.onload = java_io_BufferedReader_init,
	.super = "java/lang/Object",
	.provides = {
		"java/io/BufferedReader",
		NULL
	}
};
