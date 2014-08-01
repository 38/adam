#include <adam.h>
#include <assert.h>
int main()
{
	adam_init();

	/* Runnability test */
	assert(dalvik_loader_from_directory("test/data/AndroidAntlr") >= 0);
	sexpression_t* sexp;
	sexp_parse("[object java/lang/String]", &sexp);
	dalvik_type_t *type = dalvik_type_from_sexp(sexp);
	sexp_free(sexp);
	const char* classname = stringpool_query("antlr/ANTLRParser");
	const char* methodname = stringpool_query("treeParserSpec");
	dalvik_type_t* arglist[] = {NULL ,NULL};
	arglist[0] = type;
	
	sexpression_t* svoid;
	assert(NULL != sexp_parse("void", &svoid));
	dalvik_type_t* tvoid = dalvik_type_from_sexp(svoid);
	sexp_free(svoid);
	
	dalvik_block_t* block = dalvik_block_from_method(classname, methodname, (const dalvik_type_t**)arglist, tvoid);
	dalvik_type_free(type);
	assert(NULL != block);

	/* setup function test */
	assert(dalvik_loader_from_directory("test/cases/dalvik_block") >= 0);
	arglist[0] = NULL;
	classname = stringpool_query("TestClass");
	
	/* Case 1 */
	methodname = stringpool_query("case1");
	block = dalvik_block_from_method(classname, methodname, (const dalvik_type_t**)arglist, tvoid);
	const dalvik_method_t* method = dalvik_memberdict_get_method(classname, methodname, (const dalvik_type_t**)arglist, tvoid);
	assert(NULL != method);
	assert(NULL != block);
	assert(block->begin == method->entry);
	assert(block->end == method->entry + 3);
	assert(block->nbranches == 1);
	assert(block->branches[0].conditional == 0);
	assert(DALVIK_BLOCK_BRANCH_UNCOND_TYPE_IS_JUMP(block->branches[0]));
	dalvik_block_t* retblk = block->branches[0].block;
	assert(NULL != retblk);
	assert(1 == retblk->nbranches);
	assert(DALVIK_BLOCK_BRANCH_UNCOND_TYPE_IS_RETURN(retblk->branches[0]));

	/* Case 2 */
	methodname = stringpool_query("case2");
	block = dalvik_block_from_method(classname, methodname, (const dalvik_type_t**)arglist, tvoid);
	method = dalvik_memberdict_get_method(classname, methodname, (const dalvik_type_t**)arglist, tvoid);
	assert(NULL != block);
	assert(NULL != method);
	/* verify block0 */
	assert(block->begin == method->entry);
	assert(block->end   == method->entry + 2);
	assert(block->nbranches == 2);
	assert(block->branches[0].disabled == 0);
	assert(block->branches[0].linked == 1);
	assert(block->branches[0].conditional == 1);
	assert(block->branches[0].eq == 1);
	assert(block->branches[0].lt == 0);
	assert(block->branches[0].gt == 0);
	assert(block->branches[0].left_inst == 0);
	assert(block->branches[0].left->header.flags == 0);
	assert(block->branches[0].left->payload.uint16 == 1);
	assert(block->branches[0].right->header.flags == 0);
	assert(block->branches[0].right->payload.uint16 == 2);
	assert(block->branches[0].block != NULL);
	assert(block->branches[1].disabled == 0);
	assert(block->branches[1].linked == 1);
	assert(block->branches[1].conditional == 0);
	assert(block->branches[1].block != NULL);
	/* verify block2 */
	dalvik_block_t* block2 = block->branches[0].block;
	assert(block2->begin == method->entry + 5);
	assert(block2->end == method->entry + 6);
	assert(block2->nbranches == 1);
	assert(block2->branches[0].disabled == 0);
	assert(block2->branches[0].linked == 1);
	assert(block2->branches[0].conditional == 0);
	assert(block2->branches[0].block != NULL);
	/* verify block1 */
	dalvik_block_t* block1 = block->branches[1].block;
	assert(block1->begin == method->entry + 3);
	assert(block1->end == method->entry + 4);
	assert(block1->nbranches == 1);
	assert(block1->branches[0].disabled == 0);
	assert(block1->branches[0].linked == 1);
	assert(block1->branches[0].conditional == 0);
	assert(block1->branches[0].block != NULL);
	assert(block1->branches[0].block == block2->branches[0].block);
	/* verify block3 */
	dalvik_block_t* block3 = block1->branches[0].block;
	assert(block3->begin == method->entry + 6);
	assert(block3->end == method->entry + 6);
	assert(block3->nbranches == 1);
	assert(DALVIK_BLOCK_BRANCH_UNCOND_TYPE_IS_RETURN(block3->branches[0]));
	assert(block3->branches[0].block == NULL);
	assert(block3->branches[0].left->payload.uint16 == 1);
	assert(block3->branches[0].block == NULL);

	/* Case 3 */
	methodname = stringpool_query("case3");
	block = dalvik_block_from_method(classname, methodname, (const dalvik_type_t**)arglist, tvoid);
	method = dalvik_memberdict_get_method(classname, methodname, (const dalvik_type_t**)arglist, tvoid);
	assert(NULL != block);
	assert(NULL != method);
	/* verify block 1 */
	assert(block->begin == method->entry + 0);
	assert(block->end == method->entry + 2);
	assert(block->nbranches == 1);
	assert(block->branches[0].linked == 1);
	assert(block->branches[0].disabled == 0);
	assert(block->branches[0].conditional == 0);
	assert(block->branches[0].block != NULL);
	/* verify block 3*/
	block3 = block->branches[0].block;
	assert(block3->begin == method->entry + 6);
	assert(block3->end == method->entry + 7);
	assert(block3->nbranches == 1);
	assert(block3->branches[0].linked == 1);
	assert(block3->branches[0].disabled == 0);
	assert(DALVIK_BLOCK_BRANCH_UNCOND_TYPE_IS_JUMP(block3->branches[0]));

	/*verify block 4 */
	dalvik_block_t* block4 = block3->branches[0].block;
	assert(block4->begin == method->entry + 7);
	assert(block4->end == method->entry + 7);
	assert(block4->nbranches == 1);
	assert(DALVIK_BLOCK_BRANCH_UNCOND_TYPE_IS_RETURN(block4->branches[0]));
	assert(block4->branches[0].left->payload.uint16 == 9);
	assert(block4->branches[0].block == NULL);

	/* Case 4 */
	methodname = stringpool_query("case4");
	block = dalvik_block_from_method(classname, methodname, (const dalvik_type_t**) arglist, tvoid);
	method = dalvik_memberdict_get_method(classname, methodname, (const dalvik_type_t**) arglist, tvoid);
	assert(NULL != block);
	assert(NULL != method);
	/* verify block 1 */
	assert(block->begin == method->entry + 0);
	assert(block->end == method->entry + 1);
	assert(block->nbranches == 1);
	assert(block->branches[0].linked == 1);
	assert(block->branches[0].disabled == 0);
	assert(block->branches[0].conditional == 0);
	assert(block->branches[0].block != NULL);
	/* verify block 2 */
	block2 = block->branches[0].block;
	assert(block2->begin == method->entry + 1);
	assert(block2->end == method->entry + 2);
	assert(block2->nbranches == 1);
	assert(block2->branches[0].linked == 1);
	assert(block2->branches[0].conditional == 0);
	assert(block2->branches[0].disabled == 0);
	assert(block2->branches[0].block != NULL);
	/* verify block3 */
	block3 = block2->branches[0].block;
	assert(block3->begin == method->entry + 2);
	assert(block3->end == method->entry + 3);
	assert(block3->nbranches == 1);
	assert(block3->branches[0].linked == 1);
	assert(block3->branches[0].conditional == 0);
	assert(block3->branches[0].disabled == 0);
	
	block4 = block3->branches[0].block;
	assert(NULL != block4);
	assert(DALVIK_BLOCK_BRANCH_UNCOND_TYPE_IS_RETURN(block4->branches[0]));
	assert(block4->branches[0].block == NULL);

	/* Case 5 */
	methodname = stringpool_query("case5");
	block = dalvik_block_from_method(classname, methodname, (const dalvik_type_t**) arglist, tvoid);
	method = dalvik_memberdict_get_method(classname, methodname, (const dalvik_type_t**) arglist, tvoid);
	assert(NULL != block);
	assert(NULL != method);

	adam_finalize();
	return 0;
}
