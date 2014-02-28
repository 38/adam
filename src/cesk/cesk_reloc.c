#include <cesk/cesk_reloc.h>
#include <dalvik/dalvik_instruction.h>
/** 
 * @file cesk_reloc.c
 * @brief implementation of relocation table
 * @todo  how to reduce the memory allocation cost
 * @todo  if the conflict is really rear, try to use a bloom filter to optimize this
 * @todo test it
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
/**
 * @brief the hash code for the relocate table 
 **/
static inline hashval_t _cesk_reloc_table_entry_hashcode(uint32_t fromaddr)
{
	return fromaddr;
}
/** 
 * @brief insert an entry to the relocation table
 * @detials here we assume the table pointer is always valid
 * @return result of insertion operate
 **/
static inline int _cesk_reloc_table_insert(cesk_reloc_table_t* table, uint32_t fromaddr, uint32_t toaddr)
{
	hashval_t h = _cesk_reloc_table_entry_hashcode(fromaddr) % CESK_RELOC_TABLE_SIZE;
	cesk_reloc_table_node_t* ptr;
	for(ptr = table->htab[h]; NULL != ptr; ptr = ptr->next)
	{
		if(ptr->from == fromaddr)
		{
			LOG_DEBUG("find duplicated relocate table entry");
			break;
		}
	}
	/* if we find some entry that is already there */
	if(NULL != ptr)
	{
		if(ptr->to != toaddr)
		{
			LOG_WARNING("ooops, there's different entries in the same key. does anything goes wrong?");
			return -1;
		}
		LOG_DEBUG("there's an entry already there");
		return 0;
	}
	/* if we didn't find anything, then create a new node for this */
	cesk_reloc_table_node_t* newnode = (cesk_reloc_table_node_t*)malloc(sizeof(cesk_reloc_table_node_t));
	if(NULL == newnode)
	{
		LOG_ERROR("can not allocate memory for relocate table");
		return -1;
	}
	newnode->from = fromaddr;
	newnode->to = toaddr;
	newnode->next = table->htab[h];
	table->htab[h] = newnode;
	LOG_DEBUG("relocating address @0x%x to @0x%x", fromaddr, toaddr);
	return 0;
}
void cesk_reloc_table_free(cesk_reloc_table_t* table)
{
	if(NULL == table) return;
	int i;
	for(i = 0; i < CESK_RELOC_TABLE_SIZE; i ++)
	{
		cesk_reloc_table_node_t* ptr;
		for(ptr = table->htab[i]; NULL == ptr;)
		{
			cesk_reloc_table_node_t* old = ptr;
			ptr = ptr->next;
			free(old);
		}
	}
	free(table);
}
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
	if(NULL == ret)
	{
		LOG_WARNING("can not allocate memory for the relocation table");
		goto ERROR;
	}
	memset(ret, 0, sizeof(cesk_reloc_table_t));
	/* because we do not delete any block in the block list, so the first part in each store should 
	 * definately be couterparts
	 */
	int npages = sour->nblocks;
	/* because if there's some pages for a store that do not have conterpart in the other, 
	 * there is no collision at all
	 */
	if(npages > dest->nblocks) npages = dest->nblocks;

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
			if(sour->blocks[i]->slots[j].value == NULL ||
			   dest->blocks[i]->slots[j].value == NULL) 
				continue;

			if(sour->blocks[i]->slots[j].value == dest->blocks[i]->slots[j].value)
			{
				/* the value are actually the same one */
				continue;
			}
			if(sour->blocks[i]->slots[j].idx == dest->blocks[i]->slots[j].idx &&
			   sour->blocks[i]->slots[j].parent == dest->blocks[i]->slots[j].parent &&
			   sour->blocks[i]->slots[j].field == dest->blocks[i]->slots[j].field)
			{
				/* if they should be together */
				continue;
			}
			/* ignore the set form the source store */
			if(sour->blocks[i]->slots[j].value->type == CESK_TYPE_SET)
			{
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
													sour->blocks[i]->slots[j].parent,
													sour->blocks[i]->slots[j].field);
			if(CESK_STORE_ADDR_NULL == new_addr)
			{
				LOG_WARNING("can not allocate new address for the conflict object");
				continue;
			}
			
			/* because the p_dest might change, so we must update the value of dest now */
			dest = *p_dest;
			/* okay, build an actually relocation rule */
			uint32_t old_addr = i * CESK_STORE_BLOCK_NSLOTS + j;
			LOG_DEBUG("find relocation plan: @0x%x --> @0x%x", old_addr, new_addr);
			/* now we insert the entry to the table */
			if(_cesk_reloc_table_insert(ret, old_addr, new_addr) < 0)
			{
				LOG_WARNING("can not build relocation rule from @0x%x to @0x%x", old_addr, new_addr);
				continue;
			}
			/* then we put an empty object in that place, so that we can merge the source object later */
			cesk_value_t* newval = cesk_value_from_classpath(cesk_object_classpath(sour->blocks[i]->slots[j].value->pointer.object));
			if(NULL == newval)
			{
				LOG_ERROR("can not create new val for the relocated object");
				continue;
			}
			if(cesk_store_attach(dest, new_addr, newval) < 0)
			{
				LOG_WARNING("can not attach the source value to the destination store");
				continue;
			}
			cesk_store_release_rw(dest, new_addr);
		}
	}
	LOG_DEBUG("relocate table has been built for store@%p to store@%p", *p_dest, sour);

	return ret;
ERROR:
	LOG_ERROR("can not build object relocation table");
	cesk_reloc_table_free(ret);
	return NULL;
}

uint32_t cesk_reloc_table_look_for(const cesk_reloc_table_t* table, uint32_t addr)
{
	if(NULL == table || CESK_STORE_ADDR_IS_CONST(addr))
	{
		LOG_ERROR("invalid argument");
		return CESK_STORE_ADDR_NULL;
	}
	hashval_t h = _cesk_reloc_table_entry_hashcode(addr) % CESK_RELOC_TABLE_SIZE;
	cesk_reloc_table_node_t* ptr;
	for(ptr = table->htab[h]; NULL != ptr; ptr = ptr->next)
		if(ptr->from == addr) return ptr->to;
	/* not found */
	return addr;
}
