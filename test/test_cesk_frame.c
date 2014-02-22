#include <adam.h>
#include <assert.h>
int main()
{
	adam_init();
	dalvik_loader_from_directory("../testdata/AndroidAntlr");
	cesk_frame_t* frame = cesk_frame_new(32);
	assert(NULL != frame);
	uint32_t addr1, addr2;
	addr1 = cesk_frame_register_load(frame, dalvik_instruction_get(123), 0 ,123);
	assert(CESK_STORE_ADDR_NULL != addr1);
	addr2 = cesk_frame_store_new_object(frame, dalvik_instruction_get(123), stringpool_query("antlr/ANTLRLexer"));
	assert(CESK_STORE_ADDR_NULL != addr2);
	assert(addr1 != addr2);
	uint32_t addr3 = cesk_frame_store_new_object(frame, dalvik_instruction_get(123), stringpool_query("antlr/ANTLRLexer"));
	assert(addr2 == addr3);
	cesk_frame_free(frame);

	adam_finalize();
	return 0;
}
