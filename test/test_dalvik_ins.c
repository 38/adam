#include <stringpool.h>
#include <dalvik/dalvik_ins.h>
#include <dalvik/sexp.h>
#include <assert.h>
sexpression_t *sexp;
dalvik_instruction_t inst;
void test_move()
{
    assert(NULL != sexp_parse("(move/from16 v12,v1234)", &sexp));
    assert(0 == dalvik_instruction_from_sexp(sexp, &inst, 0, NULL));
    assert(inst.opcode == DVM_MOVE);
    assert(inst.num_oprands == 2);
    assert(inst.operands[0].payload.uint16 == 12);
    assert(inst.operands[1].payload.uint16 == 1234);
    assert(inst.operands[0].header.info.size == 0);
    sexp_free(sexp);
    
    assert(NULL != sexp_parse("(move-wide/from16 v12,v1234)", &sexp));
    assert(0 == dalvik_instruction_from_sexp(sexp, &inst, 0, NULL));
    assert(inst.opcode == DVM_MOVE);
    assert(inst.num_oprands == 2);
    assert(inst.operands[0].payload.uint16 == 12);
    assert(inst.operands[1].payload.uint16 == 1234);
    assert(inst.operands[0].header.info.size == 1);
    assert(inst.operands[1].header.info.size == 1);
    sexp_free(sexp);
    
    assert(NULL != sexp_parse("(move v12,v1234)", &sexp));
    assert(0 == dalvik_instruction_from_sexp(sexp, &inst, 0, NULL));
    assert(inst.opcode == DVM_MOVE);
    assert(inst.num_oprands == 2);
    assert(inst.operands[0].payload.uint16 == 12);
    assert(inst.operands[1].payload.uint16 == 1234);
    assert(inst.operands[0].header.info.size == 0);
    assert(inst.operands[1].header.info.size == 0);
    sexp_free(sexp);

    assert(NULL != sexp_parse("(move-result/wide v1234)", &sexp));
    assert(0 == dalvik_instruction_from_sexp(sexp, &inst, 0, NULL));
    assert(inst.opcode == DVM_MOVE);
    assert(inst.num_oprands == 2);
    assert(inst.operands[0].payload.uint16 == 1234);
    assert(inst.operands[0].header.info.size == 1);
    assert(inst.operands[1].header.info.is_result);
    sexp_free(sexp);

    assert(NULL != sexp_parse("(move-result-object v1234)", &sexp));
    assert(0 == dalvik_instruction_from_sexp(sexp, &inst, 0, NULL));
    assert(inst.opcode == DVM_MOVE);
    assert(inst.num_oprands == 2);
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
    assert(0 == dalvik_instruction_from_sexp(sexp, &inst, 0, NULL));
    assert(inst.opcode == DVM_RETURN);
    assert(inst.num_oprands == 1);
    assert(inst.operands[0].payload.uint16 == 1234);
    assert(inst.operands[0].header.info.size == 0);
    assert(inst.operands[0].header.info.type = DVM_OPERAND_TYPE_OBJECT);
    sexp_free(sexp);
    
    assert(NULL != sexp_parse("(return-void)", &sexp));
    assert(0 == dalvik_instruction_from_sexp(sexp, &inst, 0, NULL));
    assert(inst.opcode == DVM_RETURN);
    assert(inst.num_oprands == 1);
    assert(inst.operands[0].header.info.type = DVM_OPERAND_TYPE_VOID);
    sexp_free(sexp);
}
void test_const()
{
    assert(NULL != sexp_parse("(const/high16 v123,1)", &sexp));
    assert(0 == dalvik_instruction_from_sexp(sexp, &inst, 0 , NULL));
    assert(inst.opcode == DVM_CONST);
    assert(inst.num_oprands == 2);
    assert(inst.operands[0].header.flags == 0);
    assert(inst.operands[0].header.info.type == 0);
    assert(inst.operands[0].payload.uint16 == 123);
    assert(inst.operands[1].header.info.is_const == 1);
    assert(inst.operands[1].payload.uint32 == 0x10000);
    sexp_free(sexp);


    assert(NULL != sexp_parse("(const v123,1235)", &sexp));
    assert(0 == dalvik_instruction_from_sexp(sexp, &inst, 0 , NULL));
    assert(inst.opcode == DVM_CONST);
    assert(inst.num_oprands == 2);
    assert(inst.operands[0].header.flags == 0);
    assert(inst.operands[0].header.info.type == 0);
    assert(inst.operands[0].payload.uint16 == 123);
    assert(inst.operands[1].header.info.is_const == 1);
    assert(inst.operands[1].payload.uint32 == 1235);
    sexp_free(sexp);
    
    assert(NULL != sexp_parse("(const/4 v123,1235)", &sexp));
    assert(0 == dalvik_instruction_from_sexp(sexp, &inst, 0 , NULL));
    assert(inst.opcode == DVM_CONST);
    assert(inst.num_oprands == 2);
    assert(inst.operands[0].header.flags == 0);
    assert(inst.operands[0].header.info.type == 0);
    assert(inst.operands[0].payload.uint16 == 123);
    assert(inst.operands[1].header.info.is_const == 1);
    assert(inst.operands[1].payload.uint32 == 1235);
    sexp_free(sexp);
    
    assert(NULL != sexp_parse("(const/16 v123,1235)", &sexp));
    assert(0 == dalvik_instruction_from_sexp(sexp, &inst, 0 , NULL));
    assert(inst.opcode == DVM_CONST);
    assert(inst.num_oprands == 2);
    assert(inst.operands[0].header.flags == 0);
    assert(inst.operands[0].header.info.type == 0);
    assert(inst.operands[0].payload.uint16 == 123);
    assert(inst.operands[1].header.info.is_const == 1);
    assert(inst.operands[1].payload.uint32 == 1235);
    sexp_free(sexp);
    
    assert(NULL != sexp_parse("(const-wide v123,123456789)", &sexp));
    assert(0 == dalvik_instruction_from_sexp(sexp, &inst, 0 , NULL));
    assert(inst.opcode == DVM_CONST);
    assert(inst.num_oprands == 2);
    assert(inst.operands[0].header.info.size == 1);
    assert(inst.operands[0].header.info.type == 0);
    assert(inst.operands[0].payload.uint16 == 123);
    assert(inst.operands[1].header.info.is_const == 1);
    assert(inst.operands[1].payload.uint64 == 123456789);
    sexp_free(sexp);
    
    assert(NULL != sexp_parse("(const-wide v123,123456789)", &sexp));
    assert(0 == dalvik_instruction_from_sexp(sexp, &inst, 0 , NULL));
    assert(inst.opcode == DVM_CONST);
    assert(inst.num_oprands == 2);
    assert(inst.operands[0].header.info.size == 1);
    assert(inst.operands[0].header.info.type == 0);
    assert(inst.operands[0].payload.uint16 == 123);
    assert(inst.operands[1].header.info.is_const == 1);
    assert(inst.operands[1].payload.uint64 == 123456789);
    sexp_free(sexp);
    
    assert(NULL != sexp_parse("(const-wide/high16 v123,1)", &sexp));
    assert(0 == dalvik_instruction_from_sexp(sexp, &inst, 0 , NULL));
    assert(inst.opcode == DVM_CONST);
    assert(inst.num_oprands == 2);
    assert(inst.operands[0].header.info.size == 1);
    assert(inst.operands[0].header.info.type == 0);
    assert(inst.operands[0].payload.uint16 == 123);
    assert(inst.operands[1].header.info.is_const == 1);
    assert(inst.operands[1].payload.uint64 == 0x0001000000000000ull);
    sexp_free(sexp);
    
    assert(NULL != sexp_parse("(const-string v123,\"this is a test case for const-string\")", &sexp));
    assert(0 == dalvik_instruction_from_sexp(sexp, &inst, 0 , NULL));
    assert(inst.opcode == DVM_CONST);
    assert(inst.num_oprands == 2);
    assert(inst.operands[1].header.info.type == DVM_OPERAND_TYPE_STRING);
    assert(inst.operands[1].payload.string == stringpool_query("this is a test case for const-string"));
    assert(inst.operands[1].header.info.is_const == 1);
    sexp_free(sexp);
    
    assert(NULL != sexp_parse("(const-class v123,this/is/a/test/class)", &sexp));
    assert(0 == dalvik_instruction_from_sexp(sexp, &inst, 0 , NULL));
    assert(inst.opcode == DVM_CONST);
    assert(inst.num_oprands == 2);
    assert(inst.operands[1].header.info.type == DVM_OPERAND_TYPE_CLASS);
    assert(inst.operands[1].payload.string == stringpool_query("this/is/a/test/class"));
    assert(inst.operands[1].header.info.is_const == 1);
    sexp_free(sexp);
}
int main()
{
    stringpool_init(1027);
    sexp_init();

    test_move();
    test_return();
    test_const();
    
    stringpool_fianlize();
    
    return 0;
}
