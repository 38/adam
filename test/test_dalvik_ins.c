#include <stringpool.h>
#include <dalvik/dalvik_ins.h>
#include <dalvik/sexp.h>
#include <assert.h>
int main()
{
    stringpool_init(1027);
    sexp_init();
    sexpression_t *sexp;
    dalvik_instruction_t inst;

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
    
    assert(NULL != sexp_parse("(return-object v1234)", &sexp));
    assert(0 == dalvik_instruction_from_sexp(sexp, &inst, 0, NULL));
    assert(inst.opcode == DVM_RETURN);
    assert(inst.num_oprands == 1);
    assert(inst.operands[0].payload.uint16 == 1234);
    assert(inst.operands[0].header.info.size == 0);
    assert(inst.operands[0].header.info.type = DVM_OPERAND_TYPE_OBJECT);
    sexp_free(sexp);
    stringpool_fianlize();
    return 0;
}
