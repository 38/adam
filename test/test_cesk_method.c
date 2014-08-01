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

	dalvik_type_free(tint);
	dalvik_type_free(tobj);

	adam_finalize();
	return 0;
}
