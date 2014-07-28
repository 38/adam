#include <stringpool.h>
#include <dalvik/dalvik_instruction.h>
#include <sexp.h>
#include <dalvik/dalvik_tokens.h>
#include <assert.h>
#include <adam.h>
sexpression_t *sexp;
dalvik_instruction_t inst;
void test_move()
{
	assert(NULL != sexp_parse("(move/from16 v12,v1234)", &sexp));
	/*assert(0 == dalvik_instruction_from_sexp(sexp, &inst, 0, NULL));
	assert(inst.opcode == DVM_MOVE);
	assert(inst.num_operands == 2);
	assert(inst.operands[0].payload.uint16 == 12);
	assert(inst.operands[1].payload.uint16 == 1234);
	assert(inst.operands[0].header.info.size == 0);*/
	sexp_free(sexp);
	
	assert(NULL != sexp_parse("(move-wide/from16 v12,v1234)", &sexp));
	assert(0 == dalvik_instruction_from_sexp(sexp, &inst, 0));
	assert(inst.opcode == DVM_MOVE);
	assert(inst.num_operands == 2);
	assert(inst.operands[0].payload.uint16 == 12);
	assert(inst.operands[1].payload.uint16 == 1234);
	assert(inst.operands[0].header.info.size == 1);
	assert(inst.operands[1].header.info.size == 1);
	sexp_free(sexp);
	
	assert(NULL != sexp_parse("(move v12,v1234)", &sexp));
	assert(0 == dalvik_instruction_from_sexp(sexp, &inst, 0));
	assert(inst.opcode == DVM_MOVE);
	assert(inst.num_operands == 2);
	assert(inst.operands[0].payload.uint16 == 12);
	assert(inst.operands[1].payload.uint16 == 1234);
	assert(inst.operands[0].header.info.size == 0);
	assert(inst.operands[1].header.info.size == 0);
	sexp_free(sexp);

	assert(NULL != sexp_parse("(move-result/wide v1234)", &sexp));
	assert(0 == dalvik_instruction_from_sexp(sexp, &inst, 0));
	assert(inst.opcode == DVM_MOVE);
	assert(inst.num_operands == 2);
	assert(inst.operands[0].payload.uint16 == 1234);
	assert(inst.operands[0].header.info.size == 1);
	assert(inst.operands[1].header.info.is_result);
	sexp_free(sexp);

	assert(NULL != sexp_parse("(move-result-object v1234)", &sexp));
	assert(0 == dalvik_instruction_from_sexp(sexp, &inst, 0));
	assert(inst.opcode == DVM_MOVE);
	assert(inst.num_operands == 2);
	assert(inst.operands[0].payload.uint16 == 1234);
	assert(inst.operands[0].header.info.size == 0);
	assert(inst.operands[0].header.info.type = DVM_OPERAND_TYPE_OBJECT);
	assert(inst.operands[1].header.info.type = DVM_OPERAND_TYPE_OBJECT);
	assert(inst.operands[1].header.info.is_result);
	sexp_free(sexp);
}
void test_return()
{
	assert(NULL != sexp_parse("(return-object v1234)", &sexp));
	assert(0 == dalvik_instruction_from_sexp(sexp, &inst, 0));
	assert(inst.opcode == DVM_RETURN);
	assert(inst.num_operands == 1);
	assert(inst.operands[0].payload.uint16 == 1234);
	assert(inst.operands[0].header.info.size == 0);
	assert(inst.operands[0].header.info.type = DVM_OPERAND_TYPE_OBJECT);
	sexp_free(sexp);
	
	assert(NULL != sexp_parse("(return-void)", &sexp));
	assert(0 == dalvik_instruction_from_sexp(sexp, &inst, 0));
	assert(inst.opcode == DVM_RETURN);
	assert(inst.num_operands == 1);
	assert(inst.operands[0].header.info.type = DVM_OPERAND_TYPE_VOID);
	sexp_free(sexp);
}
void test_const()
{
	assert(NULL != sexp_parse("(const/high16 v123,1)", &sexp));
	assert(0 == dalvik_instruction_from_sexp(sexp, &inst, 0));
	assert(inst.opcode == DVM_CONST);
	assert(inst.num_operands == 2);
	assert(inst.operands[0].header.flags == 0);
	assert(inst.operands[0].header.info.type == 0);
	assert(inst.operands[0].payload.uint16 == 123);
	assert(inst.operands[1].header.info.is_const == 1);
	assert(inst.operands[1].payload.uint32 == 0x10000);
	sexp_free(sexp);


	assert(NULL != sexp_parse("(const v123,1235)", &sexp));
	assert(0 == dalvik_instruction_from_sexp(sexp, &inst, 0));
	assert(inst.opcode == DVM_CONST);
	assert(inst.num_operands == 2);
	assert(inst.operands[0].header.flags == 0);
	assert(inst.operands[0].header.info.type == 0);
	assert(inst.operands[0].payload.uint16 == 123);
	assert(inst.operands[1].header.info.is_const == 1);
	assert(inst.operands[1].payload.uint32 == 1235);
	sexp_free(sexp);
	
	assert(NULL != sexp_parse("(const/4 v123,1235)", &sexp));
	assert(0 == dalvik_instruction_from_sexp(sexp, &inst, 0));
	assert(inst.opcode == DVM_CONST);
	assert(inst.num_operands == 2);
	assert(inst.operands[0].header.flags == 0);
	assert(inst.operands[0].header.info.type == 0);
	assert(inst.operands[0].payload.uint16 == 123);
	assert(inst.operands[1].header.info.is_const == 1);
	assert(inst.operands[1].payload.uint32 == 1235);
	sexp_free(sexp);
	
	assert(NULL != sexp_parse("(const/16 v123,1235)", &sexp));
	assert(0 == dalvik_instruction_from_sexp(sexp, &inst, 0));
	assert(inst.opcode == DVM_CONST);
	assert(inst.num_operands == 2);
	assert(inst.operands[0].header.flags == 0);
	assert(inst.operands[0].header.info.type == 0);
	assert(inst.operands[0].payload.uint16 == 123);
	assert(inst.operands[1].header.info.is_const == 1);
	assert(inst.operands[1].payload.uint32 == 1235);
	sexp_free(sexp);
	
	assert(NULL != sexp_parse("(const-wide v123,123456789)", &sexp));
	assert(0 == dalvik_instruction_from_sexp(sexp, &inst, 0));
	assert(inst.opcode == DVM_CONST);
	assert(inst.num_operands == 2);
	assert(inst.operands[0].header.info.size == 1);
	assert(inst.operands[0].header.info.type == 0);
	assert(inst.operands[0].payload.uint16 == 123);
	assert(inst.operands[1].header.info.is_const == 1);
	assert(inst.operands[1].payload.uint64 == 123456789);
	sexp_free(sexp);
	
	assert(NULL != sexp_parse("(const-wide v123,123456789)", &sexp));
	assert(0 == dalvik_instruction_from_sexp(sexp, &inst, 0));
	assert(inst.opcode == DVM_CONST);
	assert(inst.num_operands == 2);
	assert(inst.operands[0].header.info.size == 1);
	assert(inst.operands[0].header.info.type == 0);
	assert(inst.operands[0].payload.uint16 == 123);
	assert(inst.operands[1].header.info.is_const == 1);
	assert(inst.operands[1].payload.uint64 == 123456789);
	sexp_free(sexp);
	
	assert(NULL != sexp_parse("(const-wide/high16 v123,1)", &sexp));
	assert(0 == dalvik_instruction_from_sexp(sexp, &inst, 0));
	assert(inst.opcode == DVM_CONST);
	assert(inst.num_operands == 2);
	assert(inst.operands[0].header.info.size == 1);
	assert(inst.operands[0].header.info.type == 0);
	assert(inst.operands[0].payload.uint16 == 123);
	assert(inst.operands[1].header.info.is_const == 1);
	assert(inst.operands[1].payload.uint64 == 0x0001000000000000ull);
	sexp_free(sexp);
	
	assert(NULL != sexp_parse("(const-string v123,\"this is a test case for const-string\")", &sexp));
	assert(0 == dalvik_instruction_from_sexp(sexp, &inst, 0));
	assert(inst.opcode == DVM_CONST);
	assert(inst.num_operands == 2);
	assert(inst.operands[1].header.info.type == DVM_OPERAND_TYPE_STRING);
	assert(inst.operands[1].payload.string == stringpool_query("this is a test case for const-string"));
	assert(inst.operands[1].header.info.is_const == 1);
	sexp_free(sexp);
	
	assert(NULL != sexp_parse("(const-class v123,this/is/a/test/class)", &sexp));
	assert(0 == dalvik_instruction_from_sexp(sexp, &inst, 0));
	assert(inst.opcode == DVM_CONST);
	assert(inst.num_operands == 2);
	assert(inst.operands[1].header.info.type == DVM_OPERAND_TYPE_CLASS);
	assert(inst.operands[1].payload.string == stringpool_query("this/is/a/test/class"));
	assert(inst.operands[1].header.info.is_const == 1);
	sexp_free(sexp);
}
void test_monitor()
{
	assert(NULL != sexp_parse("(monitor-enter v123)", &sexp));
	assert(0 == dalvik_instruction_from_sexp(sexp, &inst, 0));
	assert(inst.opcode == DVM_MONITOR);
	assert(inst.num_operands == 1);
	assert(inst.operands[0].header.info.type == 0);
	assert(inst.operands[0].payload.uint16 == 123);
	assert(inst.operands[0].header.info.is_const == 0);
	assert(inst.flags == DVM_FLAG_MONITOR_ENT);
	sexp_free(sexp);
	
	assert(NULL != sexp_parse("(monitor-exit v123)", &sexp));
	assert(0 == dalvik_instruction_from_sexp(sexp, &inst, 0));
	assert(inst.opcode == DVM_MONITOR);
	assert(inst.num_operands == 1);
	assert(inst.operands[0].header.info.type == 0);
	assert(inst.operands[0].payload.uint16 == 123);
	assert(inst.operands[0].header.info.is_const == 0);
	assert(inst.flags == DVM_FLAG_MONITOR_EXT);
	sexp_free(sexp);
}
void test_packed()
{
	assert(NULL != sexp_parse(
		   "(packed-switch v123, 456,\n"
		   "  l456,     ;case456\n"
		   "  l457,     ;case457\n"
		   "  l458,     ;case458\n"
		   "  ldefault  ;default\n"
		   ")", &sexp));
	assert(0 == dalvik_instruction_from_sexp(sexp, &inst, 0));
	assert(DVM_SWITCH == inst.opcode);
	assert(3 == inst.num_operands);
	assert(123 == inst.operands[0].payload.uint16);
	assert(0 == inst.operands[0].header.flags);
	assert(1 == inst.operands[1].header.info.is_const);
	assert(456 == inst.operands[1].payload.uint16);
	assert(inst.operands[1].header.info.type == DVM_OPERAND_TYPE_INT);
	assert(inst.operands[2].header.info.type == DVM_OPERAND_TYPE_LABELVECTOR);
	assert(inst.operands[2].header.info.is_const == 1);
	assert(inst.operands[2].payload.branches != NULL);
	vector_t* v = inst.operands[2].payload.branches;
	assert(0 == *(int*)vector_get(v, 0));
	assert(1 == *(int*)vector_get(v, 1));
	assert(2 == *(int*)vector_get(v, 2));
	assert(3 == *(int*)vector_get(v, 3));
	assert(0 == dalvik_label_get_label_id(stringpool_query("l456")));
	assert(1 == dalvik_label_get_label_id(stringpool_query("l457")));
	assert(2 == dalvik_label_get_label_id(stringpool_query("l458")));
	assert(3 == dalvik_label_get_label_id(stringpool_query("ldefault")));
	sexp_free(sexp);
	dalvik_instruction_free(&inst);
}
void test_sparse()
{
	assert(NULL != sexp_parse(
		   "(sparse-switch v4 \n"
		   "    (9 sp1) \n"
		   "    (10 sp2) \n"
		   "    (13 sp3) \n"
		   "    (65535 sp4)\n"
		   "    (default sp5))", 
		   &sexp));
	assert(0 == dalvik_instruction_from_sexp(sexp, &inst, 0));
	assert(inst.opcode == DVM_SWITCH);
	assert(inst.flags == DVM_FLAG_SWITCH_SPARSE);
	assert(inst.num_operands == 2);
	assert(inst.operands[0].header.flags == 0);
	assert(inst.operands[0].payload.uint16 = 4);
	assert(inst.operands[1].header.info.is_const == 1);
	assert(inst.operands[1].header.info.type == DVM_OPERAND_TYPE_SPARSE);
	assert(inst.operands[1].payload.sparse != NULL);
	vector_t* v = inst.operands[1].payload.branches;
	assert(4 ==((dalvik_sparse_switch_branch_t *)vector_get(v, 0))->labelid);
	assert(5 ==((dalvik_sparse_switch_branch_t *)vector_get(v, 1))->labelid);
	assert(6 ==((dalvik_sparse_switch_branch_t *)vector_get(v, 2))->labelid);
	assert(7 ==((dalvik_sparse_switch_branch_t *)vector_get(v, 3))->labelid);
	assert(8 ==((dalvik_sparse_switch_branch_t *)vector_get(v, 4))->labelid);

	assert(9 ==((dalvik_sparse_switch_branch_t *)vector_get(v, 0))->cond);
	assert(10 ==((dalvik_sparse_switch_branch_t *)vector_get(v, 1))->cond);
	assert(13 ==((dalvik_sparse_switch_branch_t *)vector_get(v, 2))->cond);
	assert(65535 ==((dalvik_sparse_switch_branch_t *)vector_get(v, 3))->cond);
	assert(0 ==((dalvik_sparse_switch_branch_t *)vector_get(v, 4))->cond);
	
	assert(0 ==((dalvik_sparse_switch_branch_t *)vector_get(v, 0))->is_default);
	assert(0 ==((dalvik_sparse_switch_branch_t *)vector_get(v, 1))->is_default);
	assert(0 ==((dalvik_sparse_switch_branch_t *)vector_get(v, 2))->is_default);
	assert(0 ==((dalvik_sparse_switch_branch_t *)vector_get(v, 3))->is_default);
	assert(1 ==((dalvik_sparse_switch_branch_t *)vector_get(v, 4))->is_default);

	assert(5 == vector_size(v));

	sexp_free(sexp);
	dalvik_instruction_free(&inst);
}
void test_arrayops()
{
	assert(NULL != sexp_parse("(aput v1 v2 v3)", &sexp));
	assert(0 == dalvik_instruction_from_sexp(sexp, &inst, 0));
	assert(inst.opcode == DVM_ARRAY);
	assert(inst.flags = DVM_FLAG_ARRAY_PUT);
	assert(inst.num_operands == 3);
	assert(inst.operands[0].header.flags == 0);
	assert(inst.operands[0].payload.uint16 == 1);
	assert(inst.operands[1].header.info.type == DVM_OPERAND_TYPE_OBJECT);
	assert(inst.operands[1].payload.uint16 == 2);
	assert(inst.operands[2].header.info.type == DVM_OPERAND_TYPE_INT);
	assert(inst.operands[2].payload.uint16 == 3);
	sexp_free(sexp);
	dalvik_instruction_free(&inst);

	assert(NULL != sexp_parse("(aput-object v1 v2 v3)", &sexp));
	assert(0 == dalvik_instruction_from_sexp(sexp, &inst, 0));
	assert(inst.opcode == DVM_ARRAY);
	assert(inst.flags = DVM_FLAG_ARRAY_PUT);
	assert(inst.num_operands == 3);
	assert(inst.operands[0].header.info.type == DVM_OPERAND_TYPE_OBJECT);
	assert(inst.operands[0].payload.uint16 == 1);
	assert(inst.operands[1].header.info.type == DVM_OPERAND_TYPE_OBJECT);
	assert(inst.operands[1].payload.uint16 == 2);
	assert(inst.operands[2].header.info.type == DVM_OPERAND_TYPE_INT);
	assert(inst.operands[2].payload.uint16 == 3);
	sexp_free(sexp);
	dalvik_instruction_free(&inst);
	
	assert(NULL != sexp_parse("(aget-object v1 v2 v3)", &sexp));
	assert(0 == dalvik_instruction_from_sexp(sexp, &inst, 0));
	assert(inst.opcode == DVM_ARRAY);
	assert(inst.flags = DVM_FLAG_ARRAY_GET);
	assert(inst.num_operands == 3);
	assert(inst.operands[0].header.info.type == DVM_OPERAND_TYPE_OBJECT);
	assert(inst.operands[0].payload.uint16 == 1);
	assert(inst.operands[1].header.info.type == DVM_OPERAND_TYPE_OBJECT);
	assert(inst.operands[1].payload.uint16 == 2);
	assert(inst.operands[2].header.info.type == DVM_OPERAND_TYPE_INT);
	assert(inst.operands[2].payload.uint16 == 3);
	sexp_free(sexp);
	dalvik_instruction_free(&inst);
}
void test_instanceops()
{
	assert(NULL != sexp_parse("(iput v1 v2 myclass.Property1 [array [object java.lang.String]])", &sexp));
	assert(0 == dalvik_instruction_from_sexp(sexp, &inst, 0));
	assert(inst.opcode == DVM_INSTANCE);
	assert(inst.flags = DVM_FLAG_INSTANCE_PUT);
	assert(inst.num_operands == 5);
	sexp_free(sexp);
	dalvik_instruction_free(&inst);
}
void test_invoke()
{
	assert(NULL != sexp_parse("(invoke-virtual {v1,v2,v3} this/is/a/test (int int int) int)", &sexp));
	assert(0 == dalvik_instruction_from_sexp(sexp,&inst, 0));
	assert(inst.opcode == DVM_INVOKE);
	assert(inst.flags == DVM_FLAG_INVOKE_VIRTUAL);
	assert(inst.num_operands == 7);
	//TODO: test it 
}
int main()
{
	adam_init();

	test_move();
	test_return();
	test_const();
	test_monitor();
	test_packed(); 
	test_sparse();
	test_arrayops();
	test_instanceops();
	test_invoke();

	adam_finalize();
	
	return 0;
}
