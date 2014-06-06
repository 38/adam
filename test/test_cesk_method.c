#include <assert.h>
#include <adam.h>
#include <google/profiler.h>
int main()
{
	adam_init();
	assert(0 == dalvik_loader_from_directory("./test/cases/analyzer"));
	const char* classpath = stringpool_query("testClass");
	const char* methodname = stringpool_query("sum");
	
	const dalvik_type_t  *  type[2] = {};

	const dalvik_block_t*  graph = dalvik_block_from_method(classpath, methodname, type);
	assert(NULL != graph);

	cesk_frame_t* frame = cesk_frame_new(graph->nregs);
	assert(NULL != frame);

	cesk_diff_t* ret = cesk_method_analyze(graph, frame);
	puts(cesk_diff_to_string(ret, NULL, 0));
	cesk_frame_free(frame);
	cesk_diff_free(ret);

	methodname = stringpool_query("frac");
	sexpression_t* sexp;
	sexp_parse("int", &sexp);
	dalvik_type_t * inttype = dalvik_type_from_sexp(sexp);
	type[0] = inttype;
	sexp_free(sexp);
	graph = dalvik_block_from_method(classpath, methodname, type);

	frame = cesk_frame_new(graph->nregs);
	assert(NULL != frame);

	cesk_set_push(frame->regs[CESK_FRAME_GENERAL_REG(9)], CESK_STORE_ADDR_POS);

	ret = cesk_method_analyze(graph, frame);
	puts(cesk_diff_to_string(ret, NULL, 0));
	cesk_frame_free(frame);
	cesk_diff_free(ret);


	methodname = stringpool_query("neg");
	type[0] = NULL;
	graph = dalvik_block_from_method(classpath, methodname, type);

	frame = cesk_frame_new(graph->nregs);
	assert(NULL != frame);

	ret = cesk_method_analyze(graph, frame);
	puts(cesk_diff_to_string(ret, NULL, 0));
	cesk_frame_free(frame);
	cesk_diff_free(ret);

	classpath = stringpool_query("listNode");
	methodname = stringpool_query("run");
	type[0] = NULL;
	graph = dalvik_block_from_method(classpath, methodname, type);

	frame = cesk_frame_new(graph->nregs);
	assert(NULL != frame);
	ret = cesk_method_analyze(graph, frame);
	cesk_frame_free(frame);
	cesk_diff_free(ret);

	adam_finalize();
	return 0;
}
