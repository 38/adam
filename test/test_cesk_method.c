#include <assert.h>
#include <adam.h>
int main()
{
	adam_init();
	sexpression_t* sint;
	assert(NULL != sexp_parse("int", &sint));
	dalvik_type_t* tint = dalvik_type_from_sexp(sint);
	sexp_free(sint);
	assert(NULL != tint);
	
	sexpression_t* sobj;
	assert(NULL != sexp_parse("[object treeNode]", &sobj));
	dalvik_type_t* tobj = dalvik_type_from_sexp(sobj);
	sexp_free(sobj);
	assert(NULL != tobj);
	
	assert(0 == dalvik_loader_from_directory("./test/cases/analyzer"));
	const char* classpath = stringpool_query("testClass");
	const char* methodname = stringpool_query("sum");
	cesk_frame_t* frame;
	cesk_diff_t* ret;
	
	const dalvik_type_t  *  type[2] = {};

	const dalvik_block_t*  graph;

	cesk_reloc_table_t* rtable;
	
	uint32_t val;
	graph = dalvik_block_from_method(classpath, methodname, type, tint);
	assert(NULL != graph);

	frame = cesk_frame_new(graph->nregs);
	assert(NULL != frame);

	ret = cesk_method_analyze(graph, frame, NULL, &rtable);
	//puts(cesk_diff_to_string(ret, NULL, 0));
	//assert(0 == strcmp("[][][(register v0 {@ffffff02,@ffffff04})][][]", cesk_diff_to_string(ret, NULL, 0)));
	assert(NULL != ret);
	assert(1 == ret->offset[CESK_DIFF_NTYPES] - ret->offset[0]);
	assert(ret->offset[CESK_DIFF_ALLOC + 1] - ret->offset[CESK_DIFF_ALLOC] == 0);
	assert(ret->offset[CESK_DIFF_REUSE + 1] - ret->offset[CESK_DIFF_REUSE] == 0);
	assert(ret->offset[CESK_DIFF_REG + 1] - ret->offset[CESK_DIFF_REG] == 1);
	assert(ret->offset[CESK_DIFF_STORE + 1] - ret->offset[CESK_DIFF_STORE] == 0);
	assert(ret->offset[CESK_DIFF_DEALLOC + 1] - ret->offset[CESK_DIFF_DEALLOC] == 0);
	assert(ret->data[ret->offset[CESK_DIFF_REG]].addr == 0);
	assert(cesk_set_size(ret->data[ret->offset[CESK_DIFF_REG]].arg.set) == 2);
	assert(cesk_set_contain(ret->data[ret->offset[CESK_DIFF_REG]].arg.set, CESK_STORE_ADDR_POS));
	assert(cesk_set_contain(ret->data[ret->offset[CESK_DIFF_REG]].arg.set, CESK_STORE_ADDR_ZERO));
	cesk_frame_free(frame);
	cesk_diff_free(ret);

	methodname = stringpool_query("frac");
	sexpression_t* sexp;
	sexp_parse("int", &sexp);
	dalvik_type_t * inttype = dalvik_type_from_sexp(sexp);
	type[0] = inttype;
	sexp_free(sexp);
	graph = dalvik_block_from_method(classpath, methodname, type, tint);

	frame = cesk_frame_new(graph->nregs);
	assert(NULL != frame);

	cesk_set_push(frame->regs[CESK_FRAME_GENERAL_REG(9)], CESK_STORE_ADDR_POS);

	ret = cesk_method_analyze(graph, frame, NULL, &rtable);
	//puts(cesk_diff_to_string(ret, NULL, 0));
	//assert(0 == strcmp("[][][(register v0 {@ffffff04})][][]", cesk_diff_to_string(ret, NULL, 0)));
	assert(NULL != ret);
	assert(1 == ret->offset[CESK_DIFF_NTYPES] - ret->offset[0]);
	assert(ret->offset[CESK_DIFF_ALLOC + 1] - ret->offset[CESK_DIFF_ALLOC] == 0);
	assert(ret->offset[CESK_DIFF_REUSE + 1] - ret->offset[CESK_DIFF_REUSE] == 0);
	assert(ret->offset[CESK_DIFF_REG + 1] - ret->offset[CESK_DIFF_REG] == 1);
	assert(ret->offset[CESK_DIFF_STORE + 1] - ret->offset[CESK_DIFF_STORE] == 0);
	assert(ret->offset[CESK_DIFF_DEALLOC + 1] - ret->offset[CESK_DIFF_DEALLOC] == 0);
	assert(ret->data[ret->offset[CESK_DIFF_REG]].addr == 0);
	assert(cesk_set_size(ret->data[ret->offset[CESK_DIFF_REG]].arg.set) == 1);
	assert(cesk_set_contain(ret->data[ret->offset[CESK_DIFF_REG]].arg.set, CESK_STORE_ADDR_POS));
	cesk_frame_free(frame);
	cesk_diff_free(ret);


	methodname = stringpool_query("neg");
	type[0] = NULL;
	graph = dalvik_block_from_method(classpath, methodname, type, tint);

	frame = cesk_frame_new(graph->nregs);
	assert(NULL != frame);

	ret = cesk_method_analyze(graph, frame, NULL, &rtable);
	//puts(cesk_diff_to_string(ret, NULL, 0));
	//assert(0 == strcmp("[][][(register v0 {@ffffff01})][][]", cesk_diff_to_string(ret, NULL, 0)));
	assert(NULL != ret);
	assert(1 == ret->offset[CESK_DIFF_NTYPES] - ret->offset[0]);
	assert(ret->offset[CESK_DIFF_ALLOC + 1] - ret->offset[CESK_DIFF_ALLOC] == 0);
	assert(ret->offset[CESK_DIFF_REUSE + 1] - ret->offset[CESK_DIFF_REUSE] == 0);
	assert(ret->offset[CESK_DIFF_REG + 1] - ret->offset[CESK_DIFF_REG] == 1);
	assert(ret->offset[CESK_DIFF_STORE + 1] - ret->offset[CESK_DIFF_STORE] == 0);
	assert(ret->offset[CESK_DIFF_DEALLOC + 1] - ret->offset[CESK_DIFF_DEALLOC] == 0);
	assert(ret->data[ret->offset[CESK_DIFF_REG]].addr == 0);
	assert(cesk_set_size(ret->data[ret->offset[CESK_DIFF_REG]].arg.set) == 1);
	assert(cesk_set_contain(ret->data[ret->offset[CESK_DIFF_REG]].arg.set, CESK_STORE_ADDR_NEG));
	cesk_frame_free(frame);
	cesk_diff_free(ret);


	classpath = stringpool_query("listNode");
	methodname = stringpool_query("run");
	type[0] = NULL;
	graph = dalvik_block_from_method(classpath, methodname, type, tint);

	frame = cesk_frame_new(graph->nregs);
	assert(NULL != frame);
	ret = cesk_method_analyze(graph, frame, NULL, &rtable);
	//puts(cesk_diff_to_string(ret, NULL, 0));
	//assert(0 == strcmp("[(allocate @ff000000 (objval (refcnt 2) [class listNode ((value @ff000001) (next @ff000002) )])) (allocate @ff000001 (setval (refcnt 2) {@ffffff02})) (allocate @ff000002 (setval (refcnt 2) {@ffffff02})) (allocate @ff000003 (objval (refcnt 1) [class listNode ((value @ff000004) (next @ff000005) )])) (allocate @ff000004 (setval (refcnt 1) {@ffffff02})) (allocate @ff000005 (setval (refcnt 1) {@ffffff02}))][(reuse @ff000003 1) (reuse @ff000004 1) (reuse @ff000005 1)][(register v0 {@ff000000})][(store @ff000002 (setval (refcnt 1) {@ffffff02,@ff000003})) (store @ff000004 (setval (refcnt 1) {@ffffff04,@ffffff02})) (store @ff000005 (setval (refcnt 1) {@ff000003,@ffffff02}))][]", cesk_diff_to_string(ret, NULL, 0)));
	assert(NULL != ret);
	assert(13 == ret->offset[CESK_DIFF_NTYPES] - ret->offset[0]);
	assert(ret->offset[CESK_DIFF_ALLOC + 1] - ret->offset[CESK_DIFF_ALLOC] == 6);
	assert(ret->offset[CESK_DIFF_REUSE + 1] - ret->offset[CESK_DIFF_REUSE] == 3);
	assert(ret->offset[CESK_DIFF_REG + 1] - ret->offset[CESK_DIFF_REG] == 1);
	assert(ret->offset[CESK_DIFF_STORE + 1] - ret->offset[CESK_DIFF_STORE] == 3);
	assert(ret->offset[CESK_DIFF_DEALLOC + 1] - ret->offset[CESK_DIFF_DEALLOC] == 0);

	assert(ret->data[ret->offset[CESK_DIFF_ALLOC]].addr == CESK_STORE_ADDR_RELOC_PREFIX + 0);
	assert(ret->data[ret->offset[CESK_DIFF_ALLOC]].arg.value->type == CESK_TYPE_OBJECT);
	assert(cesk_object_classpath(ret->data[ret->offset[CESK_DIFF_ALLOC]].arg.value->pointer.object) == stringpool_query("listNode"));
	assert(cesk_object_get_addr(
				ret->data[ret->offset[CESK_DIFF_ALLOC]].arg.value->pointer.object, 
				stringpool_query("listNode"), 
				stringpool_query("value"), 
				NULL, NULL, &val) == 0);
	assert(val == CESK_STORE_ADDR_RELOC_PREFIX + 1);
	assert(cesk_object_get_addr(
				ret->data[ret->offset[CESK_DIFF_ALLOC]].arg.value->pointer.object, 
				stringpool_query("listNode"), 
				stringpool_query("next"), 
				NULL, NULL, &val) == 0);
	assert(val == CESK_STORE_ADDR_RELOC_PREFIX + 2);

	assert(ret->data[ret->offset[CESK_DIFF_ALLOC] + 1].addr == CESK_STORE_ADDR_RELOC_PREFIX + 1);
	assert(ret->data[ret->offset[CESK_DIFF_ALLOC] + 1].arg.value->type == CESK_TYPE_SET);
	
	assert(ret->data[ret->offset[CESK_DIFF_ALLOC] + 2].addr == CESK_STORE_ADDR_RELOC_PREFIX + 2);
	assert(ret->data[ret->offset[CESK_DIFF_ALLOC] + 2].arg.value->type == CESK_TYPE_SET);

	assert(ret->data[ret->offset[CESK_DIFF_ALLOC] + 3].addr == CESK_STORE_ADDR_RELOC_PREFIX + 3);
	assert(ret->data[ret->offset[CESK_DIFF_ALLOC] + 3].arg.value->type == CESK_TYPE_OBJECT);
	assert(cesk_object_classpath(ret->data[ret->offset[CESK_DIFF_ALLOC] + 3].arg.value->pointer.object) == stringpool_query("listNode"));
	assert(cesk_object_get_addr(
				ret->data[ret->offset[CESK_DIFF_ALLOC] + 3].arg.value->pointer.object, 
				stringpool_query("listNode"), 
				stringpool_query("value"), 
				NULL, NULL, &val) == 0);
	assert(val == CESK_STORE_ADDR_RELOC_PREFIX + 4);
	assert(cesk_object_get_addr(
				ret->data[ret->offset[CESK_DIFF_ALLOC] + 3].arg.value->pointer.object, 
				stringpool_query("listNode"), 
				stringpool_query("next"), 
				NULL, NULL, &val) == 0);
	assert(val == CESK_STORE_ADDR_RELOC_PREFIX + 5);

	assert(ret->data[ret->offset[CESK_DIFF_ALLOC] + 4].addr == CESK_STORE_ADDR_RELOC_PREFIX + 4);
	assert(ret->data[ret->offset[CESK_DIFF_ALLOC] + 4].arg.value->type == CESK_TYPE_SET);
	
	assert(ret->data[ret->offset[CESK_DIFF_ALLOC] + 5].addr == CESK_STORE_ADDR_RELOC_PREFIX + 5);
	assert(ret->data[ret->offset[CESK_DIFF_ALLOC] + 5].arg.value->type == CESK_TYPE_SET);

	assert(ret->data[ret->offset[CESK_DIFF_REUSE] + 0].addr == CESK_STORE_ADDR_RELOC_PREFIX + 3);
	assert(ret->data[ret->offset[CESK_DIFF_REUSE] + 1].addr == CESK_STORE_ADDR_RELOC_PREFIX + 4);
	assert(ret->data[ret->offset[CESK_DIFF_REUSE] + 2].addr == CESK_STORE_ADDR_RELOC_PREFIX + 5);

	assert(ret->data[ret->offset[CESK_DIFF_REG] + 0].addr == 0);
	assert(cesk_set_size(ret->data[ret->offset[CESK_DIFF_REG] + 0].arg.set) == 1);
	assert(cesk_set_contain(ret->data[ret->offset[CESK_DIFF_REG] + 0].arg.set, CESK_STORE_ADDR_RELOC_PREFIX + 0));

	assert(ret->data[ret->offset[CESK_DIFF_STORE] + 0].addr == CESK_STORE_ADDR_RELOC_PREFIX + 2);
	assert(ret->data[ret->offset[CESK_DIFF_STORE] + 0].arg.value->type == CESK_TYPE_SET);
	assert(cesk_set_size(ret->data[ret->offset[CESK_DIFF_STORE] + 0].arg.value->pointer.set) == 2);
	assert(cesk_set_contain(ret->data[ret->offset[CESK_DIFF_STORE] + 0].arg.value->pointer.set, CESK_STORE_ADDR_RELOC_PREFIX + 3));
	assert(cesk_set_contain(ret->data[ret->offset[CESK_DIFF_STORE] + 0].arg.value->pointer.set, CESK_STORE_ADDR_ZERO));
	
	assert(ret->data[ret->offset[CESK_DIFF_STORE] + 1].addr == CESK_STORE_ADDR_RELOC_PREFIX + 4);
	assert(ret->data[ret->offset[CESK_DIFF_STORE] + 1].arg.value->type == CESK_TYPE_SET);
	assert(cesk_set_size(ret->data[ret->offset[CESK_DIFF_STORE] + 1].arg.value->pointer.set) == 2);
	assert(cesk_set_contain(ret->data[ret->offset[CESK_DIFF_STORE] + 1].arg.value->pointer.set, CESK_STORE_ADDR_POS));
	assert(cesk_set_contain(ret->data[ret->offset[CESK_DIFF_STORE] + 1].arg.value->pointer.set, CESK_STORE_ADDR_ZERO));

	assert(ret->data[ret->offset[CESK_DIFF_STORE] + 2].addr == CESK_STORE_ADDR_RELOC_PREFIX + 5);
	assert(ret->data[ret->offset[CESK_DIFF_STORE] + 2].arg.value->type == CESK_TYPE_SET);
	assert(cesk_set_size(ret->data[ret->offset[CESK_DIFF_STORE] + 2].arg.value->pointer.set) == 2);
	assert(cesk_set_contain(ret->data[ret->offset[CESK_DIFF_STORE] + 2].arg.value->pointer.set, CESK_STORE_ADDR_RELOC_PREFIX + 3));
	assert(cesk_set_contain(ret->data[ret->offset[CESK_DIFF_STORE] + 2].arg.value->pointer.set, CESK_STORE_ADDR_ZERO));
	cesk_frame_free(frame);
	cesk_diff_free(ret);
	
	classpath = stringpool_query("treeNode");
	methodname = stringpool_query("run");
	type[0] = NULL;
	graph = dalvik_block_from_method(classpath, methodname, type, tobj);
	assert(NULL != graph);

	frame = cesk_frame_new(graph->nregs);
	assert(NULL != frame);

	ret = cesk_method_analyze(graph, frame, NULL, &rtable);

	assert(NULL != ret);
	assert(12 == ret->offset[CESK_DIFF_NTYPES] - ret->offset[0]);
	assert(ret->offset[CESK_DIFF_ALLOC + 1] - ret->offset[CESK_DIFF_ALLOC] == 4);
	assert(ret->offset[CESK_DIFF_REUSE + 1] - ret->offset[CESK_DIFF_REUSE] == 4);
	assert(ret->offset[CESK_DIFF_REG + 1] - ret->offset[CESK_DIFF_REG] == 1);
	assert(ret->offset[CESK_DIFF_STORE + 1] - ret->offset[CESK_DIFF_STORE] == 3);
	assert(ret->offset[CESK_DIFF_DEALLOC + 1] - ret->offset[CESK_DIFF_DEALLOC] == 0);

	assert(ret->data[ret->offset[CESK_DIFF_ALLOC]].addr == CESK_STORE_ADDR_RELOC_PREFIX + 0);
	assert(ret->data[ret->offset[CESK_DIFF_ALLOC]].arg.value->type == CESK_TYPE_OBJECT);
	assert(cesk_object_classpath(ret->data[ret->offset[CESK_DIFF_ALLOC]].arg.value->pointer.object) == stringpool_query("treeNode"));
	assert(cesk_object_get_addr(
				ret->data[ret->offset[CESK_DIFF_ALLOC]].arg.value->pointer.object, 
				stringpool_query("treeNode"), 
				stringpool_query("key"), 
				NULL, NULL, &val) == 0);
	assert(val == CESK_STORE_ADDR_RELOC_PREFIX + 1);
	assert(cesk_object_get_addr(
				ret->data[ret->offset[CESK_DIFF_ALLOC]].arg.value->pointer.object, 
				stringpool_query("treeNode"), 
				stringpool_query("left"), 
				NULL, NULL, &val) == 0);
	assert(val == CESK_STORE_ADDR_RELOC_PREFIX + 2);
	assert(cesk_object_get_addr(
				ret->data[ret->offset[CESK_DIFF_ALLOC]].arg.value->pointer.object, 
				stringpool_query("treeNode"), 
				stringpool_query("right"), 
				NULL, NULL, &val) == 0);
	assert(val == CESK_STORE_ADDR_RELOC_PREFIX + 3);
	
	assert(ret->data[ret->offset[CESK_DIFF_ALLOC] + 1].addr == CESK_STORE_ADDR_RELOC_PREFIX + 1);
	assert(ret->data[ret->offset[CESK_DIFF_ALLOC] + 1].arg.value->type == CESK_TYPE_SET);

	assert(ret->data[ret->offset[CESK_DIFF_ALLOC] + 2].addr == CESK_STORE_ADDR_RELOC_PREFIX + 2);
	assert(ret->data[ret->offset[CESK_DIFF_ALLOC] + 2].arg.value->type == CESK_TYPE_SET);

	assert(ret->data[ret->offset[CESK_DIFF_ALLOC] + 3].addr == CESK_STORE_ADDR_RELOC_PREFIX + 3);
	assert(ret->data[ret->offset[CESK_DIFF_ALLOC] + 3].arg.value->type == CESK_TYPE_SET);

	assert(ret->data[ret->offset[CESK_DIFF_REUSE] + 0].addr == CESK_STORE_ADDR_RELOC_PREFIX + 0);
	assert(ret->data[ret->offset[CESK_DIFF_REUSE] + 1].addr == CESK_STORE_ADDR_RELOC_PREFIX + 1);
	assert(ret->data[ret->offset[CESK_DIFF_REUSE] + 2].addr == CESK_STORE_ADDR_RELOC_PREFIX + 2);
	assert(ret->data[ret->offset[CESK_DIFF_REUSE] + 3].addr == CESK_STORE_ADDR_RELOC_PREFIX + 3);

	assert(ret->data[ret->offset[CESK_DIFF_REG] + 0].addr == 0);
	assert(cesk_set_size(ret->data[ret->offset[CESK_DIFF_REG] + 0].arg.set) == 2);
	assert(cesk_set_contain(ret->data[ret->offset[CESK_DIFF_REG] + 0].arg.set, CESK_STORE_ADDR_RELOC_PREFIX + 0));
	assert(cesk_set_contain(ret->data[ret->offset[CESK_DIFF_REG] + 0].arg.set, CESK_STORE_ADDR_ZERO));

	assert(ret->data[ret->offset[CESK_DIFF_STORE] + 0].addr == CESK_STORE_ADDR_RELOC_PREFIX + 1);
	assert(ret->data[ret->offset[CESK_DIFF_STORE] + 0].arg.value->type == CESK_TYPE_SET);
	assert(cesk_set_size(ret->data[ret->offset[CESK_DIFF_STORE] + 0].arg.value->pointer.set) == 2);
	assert(cesk_set_contain(ret->data[ret->offset[CESK_DIFF_STORE] + 0].arg.value->pointer.set, CESK_STORE_ADDR_POS));
	assert(cesk_set_contain(ret->data[ret->offset[CESK_DIFF_STORE] + 0].arg.value->pointer.set, CESK_STORE_ADDR_ZERO));
	
	assert(ret->data[ret->offset[CESK_DIFF_STORE] + 1].addr == CESK_STORE_ADDR_RELOC_PREFIX + 2);
	assert(ret->data[ret->offset[CESK_DIFF_STORE] + 1].arg.value->type == CESK_TYPE_SET);
	assert(cesk_set_size(ret->data[ret->offset[CESK_DIFF_STORE] + 1].arg.value->pointer.set) == 2);
	assert(cesk_set_contain(ret->data[ret->offset[CESK_DIFF_STORE] + 1].arg.value->pointer.set, CESK_STORE_ADDR_RELOC_PREFIX + 0));
	assert(cesk_set_contain(ret->data[ret->offset[CESK_DIFF_STORE] + 1].arg.value->pointer.set, CESK_STORE_ADDR_ZERO));

	assert(ret->data[ret->offset[CESK_DIFF_STORE] + 2].addr == CESK_STORE_ADDR_RELOC_PREFIX + 3);
	assert(ret->data[ret->offset[CESK_DIFF_STORE] + 2].arg.value->type == CESK_TYPE_SET);
	assert(cesk_set_size(ret->data[ret->offset[CESK_DIFF_STORE] + 2].arg.value->pointer.set) == 2);
	assert(cesk_set_contain(ret->data[ret->offset[CESK_DIFF_STORE] + 2].arg.value->pointer.set, CESK_STORE_ADDR_RELOC_PREFIX + 0));
	assert(cesk_set_contain(ret->data[ret->offset[CESK_DIFF_STORE] + 2].arg.value->pointer.set, CESK_STORE_ADDR_ZERO));
	cesk_frame_free(frame);
	cesk_diff_free(ret);

	classpath = stringpool_query("Main");
	methodname = stringpool_query("main");
	type[0] = NULL;
	graph = dalvik_block_from_method(classpath, methodname, type, tobj);
	assert(NULL != graph);

	frame = cesk_frame_new(graph->nregs);
	assert(NULL != frame);

	ret = cesk_method_analyze(graph, frame, NULL, &rtable);

	assert(NULL != ret);
	
	assert(13 == ret->offset[CESK_DIFF_NTYPES] - ret->offset[0]);
	assert(ret->offset[CESK_DIFF_ALLOC + 1] - ret->offset[CESK_DIFF_ALLOC] == 4);
	assert(ret->offset[CESK_DIFF_REUSE + 1] - ret->offset[CESK_DIFF_REUSE] == 4);
	assert(ret->offset[CESK_DIFF_REG + 1] - ret->offset[CESK_DIFF_REG] == 2);
	assert(ret->offset[CESK_DIFF_STORE + 1] - ret->offset[CESK_DIFF_STORE] == 3);
	assert(ret->offset[CESK_DIFF_DEALLOC + 1] - ret->offset[CESK_DIFF_DEALLOC] == 0);
	
	assert(ret->data[ret->offset[CESK_DIFF_ALLOC]].addr == CESK_STORE_ADDR_RELOC_PREFIX + 0);
	assert(ret->data[ret->offset[CESK_DIFF_ALLOC]].arg.value->type == CESK_TYPE_OBJECT);
	assert(cesk_object_classpath(ret->data[ret->offset[CESK_DIFF_ALLOC]].arg.value->pointer.object) == stringpool_query("treeNode"));
	assert(cesk_object_get_addr(
				ret->data[ret->offset[CESK_DIFF_ALLOC]].arg.value->pointer.object, 
				stringpool_query("treeNode"), 
				stringpool_query("key"), 
				NULL, NULL, &val) == 0);
	assert(val == CESK_STORE_ADDR_RELOC_PREFIX + 1);
	assert(cesk_object_get_addr(
				ret->data[ret->offset[CESK_DIFF_ALLOC]].arg.value->pointer.object, 
				stringpool_query("treeNode"), 
				stringpool_query("left"), 
				NULL, NULL, &val) == 0);
	assert(val == CESK_STORE_ADDR_RELOC_PREFIX + 2);
	assert(cesk_object_get_addr(
				ret->data[ret->offset[CESK_DIFF_ALLOC]].arg.value->pointer.object, 
				stringpool_query("treeNode"), 
				stringpool_query("right"), 
				NULL, NULL, &val) == 0);
	assert(val == CESK_STORE_ADDR_RELOC_PREFIX + 3);
	
	assert(ret->data[ret->offset[CESK_DIFF_ALLOC] + 1].addr == CESK_STORE_ADDR_RELOC_PREFIX + 1);
	assert(ret->data[ret->offset[CESK_DIFF_ALLOC] + 1].arg.value->type == CESK_TYPE_SET);

	assert(ret->data[ret->offset[CESK_DIFF_ALLOC] + 2].addr == CESK_STORE_ADDR_RELOC_PREFIX + 2);
	assert(ret->data[ret->offset[CESK_DIFF_ALLOC] + 2].arg.value->type == CESK_TYPE_SET);

	assert(ret->data[ret->offset[CESK_DIFF_ALLOC] + 3].addr == CESK_STORE_ADDR_RELOC_PREFIX + 3);
	assert(ret->data[ret->offset[CESK_DIFF_ALLOC] + 3].arg.value->type == CESK_TYPE_SET);

	assert(ret->data[ret->offset[CESK_DIFF_REUSE] + 0].addr == CESK_STORE_ADDR_RELOC_PREFIX + 0);
	assert(ret->data[ret->offset[CESK_DIFF_REUSE] + 1].addr == CESK_STORE_ADDR_RELOC_PREFIX + 1);
	assert(ret->data[ret->offset[CESK_DIFF_REUSE] + 2].addr == CESK_STORE_ADDR_RELOC_PREFIX + 2);
	assert(ret->data[ret->offset[CESK_DIFF_REUSE] + 3].addr == CESK_STORE_ADDR_RELOC_PREFIX + 3);

	assert(ret->data[ret->offset[CESK_DIFF_REG] + 0].addr == 0);
	assert(cesk_set_size(ret->data[ret->offset[CESK_DIFF_REG] + 0].arg.set) == 2);
	assert(cesk_set_contain(ret->data[ret->offset[CESK_DIFF_REG] + 0].arg.set, CESK_STORE_ADDR_RELOC_PREFIX + 0));
	assert(cesk_set_contain(ret->data[ret->offset[CESK_DIFF_REG] + 0].arg.set, CESK_STORE_ADDR_ZERO));
	
	assert(ret->data[ret->offset[CESK_DIFF_REG] + 1].addr == (CESK_FRAME_REG_STATIC_PREFIX | 0));
	assert(cesk_set_size(ret->data[ret->offset[CESK_DIFF_REG] + 1].arg.set) == 2);
	assert(cesk_set_contain(ret->data[ret->offset[CESK_DIFF_REG] + 1].arg.set, CESK_STORE_ADDR_RELOC_PREFIX + 0));
	assert(cesk_set_contain(ret->data[ret->offset[CESK_DIFF_REG] + 1].arg.set, CESK_STORE_ADDR_ZERO));

	assert(ret->data[ret->offset[CESK_DIFF_STORE] + 0].addr == CESK_STORE_ADDR_RELOC_PREFIX + 1);
	assert(ret->data[ret->offset[CESK_DIFF_STORE] + 0].arg.value->type == CESK_TYPE_SET);
	assert(cesk_set_size(ret->data[ret->offset[CESK_DIFF_STORE] + 0].arg.value->pointer.set) == 2);
	assert(cesk_set_contain(ret->data[ret->offset[CESK_DIFF_STORE] + 0].arg.value->pointer.set, CESK_STORE_ADDR_POS));
	assert(cesk_set_contain(ret->data[ret->offset[CESK_DIFF_STORE] + 0].arg.value->pointer.set, CESK_STORE_ADDR_ZERO));
	
	assert(ret->data[ret->offset[CESK_DIFF_STORE] + 1].addr == CESK_STORE_ADDR_RELOC_PREFIX + 2);
	assert(ret->data[ret->offset[CESK_DIFF_STORE] + 1].arg.value->type == CESK_TYPE_SET);
	assert(cesk_set_size(ret->data[ret->offset[CESK_DIFF_STORE] + 1].arg.value->pointer.set) == 2);
	assert(cesk_set_contain(ret->data[ret->offset[CESK_DIFF_STORE] + 1].arg.value->pointer.set, CESK_STORE_ADDR_RELOC_PREFIX + 0));
	assert(cesk_set_contain(ret->data[ret->offset[CESK_DIFF_STORE] + 1].arg.value->pointer.set, CESK_STORE_ADDR_ZERO));

	assert(ret->data[ret->offset[CESK_DIFF_STORE] + 2].addr == CESK_STORE_ADDR_RELOC_PREFIX + 3);
	assert(ret->data[ret->offset[CESK_DIFF_STORE] + 2].arg.value->type == CESK_TYPE_SET);
	assert(cesk_set_size(ret->data[ret->offset[CESK_DIFF_STORE] + 2].arg.value->pointer.set) == 2);
	assert(cesk_set_contain(ret->data[ret->offset[CESK_DIFF_STORE] + 2].arg.value->pointer.set, CESK_STORE_ADDR_RELOC_PREFIX + 0));
	assert(cesk_set_contain(ret->data[ret->offset[CESK_DIFF_STORE] + 2].arg.value->pointer.set, CESK_STORE_ADDR_ZERO));
	cesk_frame_free(frame);
	cesk_diff_free(ret); 
	
	classpath = stringpool_query("virtualTest");
	methodname = stringpool_query("Case1");
	type[0] = tint;
	graph = dalvik_block_from_method(classpath, methodname, type, tint);
	assert(NULL != graph);

	frame = cesk_frame_new(graph->nregs);
	cesk_set_push(frame->regs[9], CESK_STORE_ADDR_ZERO);
	cesk_set_push(frame->regs[9], CESK_STORE_ADDR_NEG);
	cesk_set_push(frame->regs[9], CESK_STORE_ADDR_POS);
	assert(NULL != frame);

	ret = cesk_method_analyze(graph, frame, NULL, &rtable);

	assert(NULL != ret);

	/* [][][(register f1 {@0xffffff04}) (register f2 {@0xffffff01}) (register f3 {@0xffffff01,@0xffffff04})][][] */
	assert(3 == ret->offset[CESK_DIFF_NTYPES] - ret->offset[0]);
	assert(ret->offset[CESK_DIFF_ALLOC + 1] - ret->offset[CESK_DIFF_ALLOC] == 0);
	assert(ret->offset[CESK_DIFF_REUSE + 1] - ret->offset[CESK_DIFF_REUSE] == 0);
	assert(ret->offset[CESK_DIFF_REG + 1] - ret->offset[CESK_DIFF_REG] == 3);
	assert(ret->offset[CESK_DIFF_STORE + 1] - ret->offset[CESK_DIFF_STORE] == 0);
	assert(ret->offset[CESK_DIFF_DEALLOC + 1] - ret->offset[CESK_DIFF_DEALLOC] == 0);

	assert(ret->data[ret->offset[CESK_DIFF_REG] + 0].addr == (CESK_FRAME_REG_STATIC_PREFIX | 1));
	assert(cesk_set_size(ret->data[ret->offset[CESK_DIFF_REG] + 0].arg.set) == 1);
	assert(cesk_set_contain(ret->data[ret->offset[CESK_DIFF_REG] + 0].arg.set, CESK_STORE_ADDR_POS));
	assert(ret->data[ret->offset[CESK_DIFF_REG] + 1].addr == (CESK_FRAME_REG_STATIC_PREFIX | 2));
	assert(cesk_set_size(ret->data[ret->offset[CESK_DIFF_REG] + 1].arg.set) == 1);
	assert(cesk_set_contain(ret->data[ret->offset[CESK_DIFF_REG] + 1].arg.set, CESK_STORE_ADDR_NEG));
	assert(ret->data[ret->offset[CESK_DIFF_REG] + 2].addr == (CESK_FRAME_REG_STATIC_PREFIX | 3));
	assert(cesk_set_size(ret->data[ret->offset[CESK_DIFF_REG] + 2].arg.set) == 2);
	assert(cesk_set_contain(ret->data[ret->offset[CESK_DIFF_REG] + 2].arg.set, CESK_STORE_ADDR_POS));
	assert(cesk_set_contain(ret->data[ret->offset[CESK_DIFF_REG] + 2].arg.set, CESK_STORE_ADDR_NEG));
	
	cesk_frame_free(frame);
	cesk_diff_free(ret);
/*[(allocate @0xff000000 (objval (refcnt 4) [class TestClass1 ()][class BaseClass ()][class java/lang/Object ()])) (allocate @0xff000001 (objval (refcnt 1) [class TestClass2 ((_value @0xff000002) )][class BaseClass ()][class java/lang/Object ()])) (allocate @0xff000002 (setval (refcnt 2) {@0xffffff02})) (allocate @0xff000003 (objval (refcnt 1) [class TestClass2 ((_value @0xff000004) )][class BaseClass ()][class java/lang/Object ()])) (allocate @0xff000004 (setval (refcnt 1) {@0xffffff02})) (allocate @0xff000005 (objval (refcnt 1) [class TestClass1 ()][class BaseClass ()][class java/lang/Object ()]))][(reuse @0xff000000 1) (reuse @0xff000001 1) (reuse @0xff000002 1)][(register v0 {@0xff000005,@0xff000003}) (register f4 {@0xff000000,@0xff000001})][(store @0xff000002 (setval (refcnt 1) {@0xffffff01})) (store @0xff000004 (setval (refcnt 1) {@0xffffff01}))][]*/
	classpath = stringpool_query("virtualTest");
	methodname = stringpool_query("Case2");
	type[0] = tint;
	graph = dalvik_block_from_method(classpath, methodname, type, tint);
	assert(NULL != graph);

	frame = cesk_frame_new(graph->nregs);
	cesk_set_push(frame->regs[9], CESK_STORE_ADDR_ZERO);
	cesk_set_push(frame->regs[9], CESK_STORE_ADDR_NEG);
	cesk_set_push(frame->regs[9], CESK_STORE_ADDR_POS);
	assert(NULL != frame);

	ret = cesk_method_analyze(graph, frame, NULL, &rtable);

	assert(NULL != ret);

	//puts(cesk_diff_to_string(ret, NULL, 0));
	assert(ret->offset[CESK_DIFF_ALLOC + 1] - ret->offset[CESK_DIFF_ALLOC] == 6);
	assert(ret->offset[CESK_DIFF_REG + 1] - ret->offset[CESK_DIFF_REG] == 2);
	assert(ret->offset[CESK_DIFF_STORE + 1] - ret->offset[CESK_DIFF_STORE] == 2);
	assert(ret->offset[CESK_DIFF_DEALLOC + 1] - ret->offset[CESK_DIFF_DEALLOC] == 0);

	uint32_t cloned_TestClass1 = CESK_STORE_ADDR_NULL, cloned_TestClass2 = CESK_STORE_ADDR_NULL, cloned_ValueSet = CESK_STORE_ADDR_NULL;
	uint32_t origin_TestClass1 = CESK_STORE_ADDR_NULL, origin_TestClass2 = CESK_STORE_ADDR_NULL, origin_ValueSet = CESK_STORE_ADDR_NULL;
	cesk_set_iter_t iter;
	uint32_t addr;
	/* first we need to check register v0 */
	assert(ret->data[ret->offset[CESK_DIFF_REG]].addr == 0);
	assert(2 == cesk_set_size(ret->data[ret->offset[CESK_DIFF_REG]].arg.set));
	assert(cesk_set_iter(ret->data[ret->offset[CESK_DIFF_REG]].arg.set, &iter));
	while(CESK_STORE_ADDR_NULL != (addr = cesk_set_iter_next(&iter)))
	{
		uint32_t i;
		for(i = ret->offset[CESK_DIFF_ALLOC]; i < ret->offset[CESK_DIFF_ALLOC + 1]; i ++)
			if(ret->data[i].addr == addr) break;
		assert(i != ret->offset[CESK_DIFF_ALLOC + 1]);
		assert(ret->data[i].arg.value->type == CESK_TYPE_OBJECT);
		const char* classpath = cesk_object_classpath(ret->data[i].arg.value->pointer.object);
		if(stringpool_query("TestClass1") == classpath) cloned_TestClass1 = addr;
		else if(stringpool_query("TestClass2") == classpath) 
		{
			cloned_TestClass2 = addr;
			assert(cesk_object_get_addr(
				ret->data[i].arg.value->pointer.object, 
				stringpool_query("TestClass2"), 
				stringpool_query("_value"), 
				NULL, NULL, &cloned_ValueSet) == 0);
			for(i = ret->offset[CESK_DIFF_ALLOC]; i < ret->offset[CESK_DIFF_ALLOC + 1]; i ++)
				if(ret->data[i].addr == cloned_ValueSet) break;
			if(i == ret->offset[CESK_DIFF_ALLOC + 1]) cloned_ValueSet = CESK_STORE_ADDR_NULL;
		}
	}
	assert(CESK_STORE_ADDR_NULL != cloned_TestClass1);
	assert(CESK_STORE_ADDR_NULL != cloned_TestClass2);
	assert(CESK_STORE_ADDR_NULL != cloned_ValueSet);
	
	/* then check f4 */	
	assert(ret->data[ret->offset[CESK_DIFF_REG] + 1].addr == CESK_FRAME_REG_STATIC_PREFIX + 4);
	assert(2 == cesk_set_size(ret->data[ret->offset[CESK_DIFF_REG] + 1].arg.set));
	assert(cesk_set_iter(ret->data[ret->offset[CESK_DIFF_REG] + 1].arg.set, &iter));
	while(CESK_STORE_ADDR_NULL != (addr = cesk_set_iter_next(&iter)))
	{
		uint32_t i;
		for(i = ret->offset[CESK_DIFF_ALLOC]; i < ret->offset[CESK_DIFF_ALLOC + 1]; i ++)
			if(ret->data[i].addr == addr) break;
		assert(i != ret->offset[CESK_DIFF_ALLOC + 1]);
		assert(ret->data[i].arg.value->type == CESK_TYPE_OBJECT);
		const char* classpath = cesk_object_classpath(ret->data[i].arg.value->pointer.object);
		if(stringpool_query("TestClass1") == classpath) origin_TestClass1 = addr;
		else if(stringpool_query("TestClass2") == classpath) 
		{
			origin_TestClass2 = addr;
			assert(cesk_object_get_addr(
				ret->data[i].arg.value->pointer.object, 
				stringpool_query("TestClass2"), 
				stringpool_query("_value"), 
				NULL, NULL, &origin_ValueSet) == 0);
			for(i = ret->offset[CESK_DIFF_ALLOC]; i < ret->offset[CESK_DIFF_ALLOC + 1]; i ++)
				if(ret->data[i].addr == origin_ValueSet) break;
			if(i == ret->offset[CESK_DIFF_ALLOC + 1]) origin_ValueSet = CESK_STORE_ADDR_NULL;
		}
	}
	assert(CESK_STORE_ADDR_NULL != origin_TestClass1);
	assert(CESK_STORE_ADDR_NULL != origin_TestClass2);
	assert(CESK_STORE_ADDR_NULL != origin_ValueSet);
	assert(origin_TestClass1 != cloned_TestClass1);
	assert(origin_TestClass2 != cloned_TestClass2);
	assert(origin_ValueSet != cloned_ValueSet);

	cesk_frame_free(frame);
	cesk_diff_free(ret); 

	dalvik_type_free(tint);
	dalvik_type_free(tobj);

	adam_finalize();
	return 0;
}
