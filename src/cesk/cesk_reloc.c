#include <cesk/cesk_reloc.h>
#include <dalvik/dalvik_instruction.h>
/** 
 * @file cesk_reloc.c
 * @brief implementation of relocation table
 * @todo  how to reduce the memory allocation cost
 */
/**
 * @brief the relocate table node 
 */
typedef struct _cesk_reloc_table_node_t {
	uint32_t from;   /*!<from this address*/
	uint32_t to;	 /*!<map to this address*/
	struct _cesk_reloc_table_node_t* next;  /*!<used for hashing*/
} cesk_reloc_table_node_t;
struct _cesk_reloc_table_t {
	cesk_reloc_table_node_t* htab[CESK_RELOC_TABLE_SIZE];
}; 
cesk_reloc_table_t* cesk_reloc_table_from_store(cesk_store_t** p_dest, const cesk_store_t* sour)
{
	cesk_reloc_table_t*  ret = NULL;
	if(NULL ==  p_dest || NULL == sour || NULL == *p_dest)
	{
		LOG_WARNING("invalid argument");
		goto ERROR;
	}
	cesk_store_t* dest = *p_dest;

	ret = (cesk_reloc_table_t*)malloc(sizeof(cesk_reloc_table_t));
	/* because we do not delete any block in the block list, so the first part in each store should 
	 * definately be couterparts
	 */
	int npages = sour->npages;
	/* because if there's some pages for a store that do not have conterpart in the other, 
	 * there is no collision at all
	 */
	if(npages > dest->npages) npages = dest->pages;

	int i;
	for(i = 0; i < npages; i ++)
	{
		if(sour->blocks[i] == dest->blocks[i])
		{
			/* two block are actually the same, there's nothing different at all */
			continue;
		}
		int j;
		for(j = 0; j < CESK_STORE_BLOCK_NSLOTS; j ++)
		{
			if(sour->blocks[i]->slots[j]->value == dest->blocks[i]->slots[j]->value)
			{
				/* the value are actually the same one */
				continue;
			}
			if(sour->blocks[i]->slots[j].idx == dest->blocks[i]->slots[j].idx &&
			   sour->blocks[i]->slots[j].parent == dest->block[i]->slots[j].parent &&
			   sour->blocks[i]->slots[j].field == dest->block[i]->slots[j].field)
			{
				/* if they should be together */
				continue;
			}
			/* the remaining is the address should not be same but actually they does */
			const dalvik_instruction_t* inst = dalvik_instruction_get(sour->blocks[i]->slots[j].idx);
			if(NULL == inst)
			{
				LOG_WARNING("can not get instruction which id = %d, ignore this one", sour->blocks[i]->slots[j].idx);
				continue;
			}
			/* we need to get the new address in the store */
			uint32_t new_addr = cesk_store_allocate(p_dest, inst, 
													sour->blocks[i]->slots[j].idx, 
													sour->blocks[i]->slots[j].parent,
													sour->blocks[i]->slots[j].field);
			if(CESK_STORE_ADDR_NULL == new_addr)
			{
				LOG_WARNING("can not allocate new address for the conflict object");
				continue;
			}
			LOG_DEBUG("find relocation plan: @0x%x --> @0x%x", i * CESK_STORE_BLOCK_NSLOTS + j, new_addr);
			//TODO: add to the allocation table
		}
	}
ERROR:
	LOG_ERROR("can not build object relocation table");
	if(NULL != ret)
	{
		free(ret);
	}
	return -1;
}

