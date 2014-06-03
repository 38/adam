#include <assert.h>
#include <adam.h>
int main()
{
	adam_init();
	assert(0 == dalvik_loader_from_directory("./test/cases/analyzer"));
	const char* classpath = stringpool_query("testClass");
	const char* methodname = stringpool_query("sum");
	const dalvik_type_t  * const type[] = {NULL};

	const dalvik_block_t*  graph = dalvik_block_from_method(classpath, methodname, type);
	assert(NULL != graph);

	cesk_frame_t* frame = cesk_frame_new(graph->nregs);
	assert(NULL != frame);

	cesk_method_analyze(graph, frame);

	cesk_frame_free(frame);

	adam_finalize();
	return 0;
}
