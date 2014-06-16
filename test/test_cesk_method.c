#include <assert.h>
#include <adam.h>
#include <google/profiler.h>
int main()
{
	adam_init();
	assert(0 == dalvik_loader_from_directory("./test/cases/analyzer"));
	const char* classpath = stringpool_query("testClass");
	const char* methodname = stringpool_query("sum");
	cesk_frame_t* frame;
	cesk_diff_t* ret;
	
	const dalvik_type_t  *  type[2] = {};

	const dalvik_block_t*  graph;

	cesk_reloc_table_t* rtable;
	
	graph = dalvik_block_from_method(classpath, methodname, type);
	assert(NULL != graph);

	frame = cesk_frame_new(graph->nregs);
	assert(NULL != frame);

	ret = cesk_method_analyze(graph, frame, NULL, &rtable);
	puts(cesk_diff_to_string(ret, NULL, 0));
	assert(0 == strcmp("[][][(register v0 {@ffffff02,@ffffff04})][][]", cesk_diff_to_string(ret, NULL, 0)));
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

	ret = cesk_method_analyze(graph, frame, NULL, &rtable);
	puts(cesk_diff_to_string(ret, NULL, 0));
	assert(0 == strcmp("[][][(register v0 {@ffffff04})][][]", cesk_diff_to_string(ret, NULL, 0)));
	cesk_frame_free(frame);
	cesk_diff_free(ret);


	methodname = stringpool_query("neg");
	type[0] = NULL;
	graph = dalvik_block_from_method(classpath, methodname, type);

	frame = cesk_frame_new(graph->nregs);
	assert(NULL != frame);

	ret = cesk_method_analyze(graph, frame, NULL, &rtable);
	puts(cesk_diff_to_string(ret, NULL, 0));
	assert(0 == strcmp("[][][(register v0 {@ffffff01})][][]", cesk_diff_to_string(ret, NULL, 0)));
	cesk_frame_free(frame);
	cesk_diff_free(ret);

	classpath = stringpool_query("listNode");
	methodname = stringpool_query("run");
	type[0] = NULL;
	graph = dalvik_block_from_method(classpath, methodname, type);

	frame = cesk_frame_new(graph->nregs);
	assert(NULL != frame);
	ret = cesk_method_analyze(graph, frame, NULL, &rtable);
	puts(cesk_diff_to_string(ret, NULL, 0));
	assert(0 == strcmp("[(allocate @ff000000 (objval (refcnt 2) [class listNode ((value @ff000001) (next @ff000002) )])) (allocate @ff000001 (setval (refcnt 2) {@ffffff02})) (allocate @ff000002 (setval (refcnt 2) {@ffffff02})) (allocate @ff000003 (objval (refcnt 1) [class listNode ((value @ff000004) (next @ff000005) )])) (allocate @ff000004 (setval (refcnt 1) {@ffffff02})) (allocate @ff000005 (setval (refcnt 1) {@ffffff02}))][(reuse @ff000003 1) (reuse @ff000004 1) (reuse @ff000005 1)][(register v0 {@ff000000})][(store @ff000002 (setval (refcnt 1) {@ffffff02,@ff000003})) (store @ff000004 (setval (refcnt 1) {@ffffff04,@ffffff02})) (store @ff000005 (setval (refcnt 1) {@ff000003,@ffffff02}))][]", cesk_diff_to_string(ret, NULL, 0)));
	cesk_frame_free(frame);
	cesk_diff_free(ret);
	
	classpath = stringpool_query("treeNode");
	methodname = stringpool_query("run");
	type[0] = NULL;
	graph = dalvik_block_from_method(classpath, methodname, type);

	frame = cesk_frame_new(graph->nregs);
	assert(NULL != frame);

	ret = cesk_method_analyze(graph, frame, NULL, &rtable);

	puts(cesk_diff_to_string(ret, NULL, 0));
	cesk_frame_free(frame);
	cesk_diff_free(ret);
	
	adam_finalize();
	return 0;
}
