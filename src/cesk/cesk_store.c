#include <string.h>
#include <inttypes.h>
#include <stdio.h>

#include <log.h>

#include <cesk/cesk_store.h>

#define HASH_INC(addr,val,reuse) ((addr * MH_MULTIPLY + cesk_value_hashcode(val))^((reuse) * ~MH_MULTIPLY))
#define HASH_CMP(addr,val,reuse) ((addr * MH_MULTIPLY + cesk_value_compute_hashcode(val))^((reuse) * ~MH_MULTIPLY))
/** 
 * @brief make a copy of a store block, but *do not touch store-block refcnt*
 * @param block store to copy
 * @note caller is responsible for update the ref-counts
 **/
static inline cesk_store_block_t* _cesk_store_block_duplicate(cesk_store_block_t* block)
{
	/* copy the store block */
	cesk_store_block_t* new_block = (cesk_store_block_t*)malloc(CESK_STORE_BLOCK_SIZE);
	if(NULL == new_block) 
	{
		LOG_ERROR("can not allocate memory for new block");
		return NULL;
	}
	memcpy(new_block, block, CESK_STORE_BLOCK_SIZE);
	new_block->refcnt = 0;
	/* increase the reference counter of the vlaues in the block */
	int i;
	for(i = 0; i < CESK_STORE_BLOCK_NSLOTS; i ++)
		if(new_block->slots[i].value != NULL)
			cesk_value_incref(new_block->slots[i].value);
	return new_block;
}
/** 
 * @brief touch refcnt befofe actual deletion , decrease intra-frame refcnt for all members of the set before free it 
 **/
static inline int _cesk_store_free_set(cesk_store_t* store, cesk_set_t* set)
{
	cesk_set_iter_t iter;
	if(NULL == cesk_set_iter(set, &iter))
	{
		LOG_ERROR("can not acquire iterator for the set");
		return -1;
	}
	uint32_t addr;
	while(CESK_STORE_ADDR_NULL != (addr = cesk_set_iter_next(&iter)))
	{
		/* decref for all its references */
		if(cesk_store_decref(store, addr) < 0)
		{
			LOG_WARNING("can not decref at address %x", addr);
		}
	}
	return 0;
}
/** 
 * @brief decrease intra-frame refcnt for all members of the object before free it 
 **/
static inline int _cesk_store_free_object(cesk_store_t* store, cesk_object_t* object)
{
	if(NULL == object)
	{
		LOG_ERROR("can not dispose a NULL object");
		return -1;
	}
	cesk_object_struct_t* this = object->members;
	int i;
	for(i = 0; i < object->depth; i ++)
	{
		int j;
		if(!this->built_in)
		{
			for(j = 0; j < this->num_members; j ++)
			{
				/* decrease the intra-store refcount */
				if(cesk_store_decref(store, this->addrtab[j]) < 0)
				{
					LOG_WARNING("can not decref at address "PRSAddr"", this->addrtab[j]);
				}
			}
		}
		else
		{
			uint32_t offset = 0;
			uint32_t buf[128];
			for(;;)
			{
				int rc = bci_class_read(this->bcidata, offset, buf, sizeof(buf)/sizeof(buf[0]), this->class.bci->class);
				if(rc < 0)
				{
					LOG_WARNING("can not get the address list, ignoring");
					break;
				}
				if(0 == rc) break;
				offset += rc;
				int j;
				for(j = 0; j < rc; j ++)
					if(cesk_store_decref(store, buf[j]) < 0)
					{
						LOG_WARNING("can not decref address at "PRSAddr"", buf[j]);
					}
			}
		}
		CESK_OBJECT_STRUCT_ADVANCE(this);
	}
	return 0;
}
/** 
 * @brief make an address empty, but do not affect the intra-frame refcnt to this address 
 **/
static inline int _cesk_store_swipe(cesk_store_t* store, cesk_store_block_t* block, uint32_t addr)
{
	uint32_t ofs = addr % CESK_STORE_BLOCK_NSLOTS;
	cesk_value_t* value = block->slots[ofs].value;
	/* release the address */
	block->slots[ofs].value = NULL; 
	
	/* update the hashcode */
	store->hashcode ^= HASH_INC(addr, value, block->slots[ofs].reuse);
	
	/* decref of its refernces */
	int rc = -1;
	switch(value->type)
	{
		case CESK_TYPE_OBJECT:
			rc = _cesk_store_free_object(store, value->pointer.object);
			break;
		case CESK_TYPE_SET:
			rc = _cesk_store_free_set(store, value->pointer.set);
			break;
		default:
			rc = -1;
			LOG_WARNING("invalid object type code %u", value->type);
	}
	if(rc < 0) LOG_WARNING("can not dispose the object");
	/* unreference from this frame */
	cesk_value_decref(value);
	return 0;
}
/** @brief get a block in a store and prepare to write */
static inline cesk_store_block_t* _cesk_store_getblock_rw(cesk_store_t* store, uint32_t addr)
{
	if(NULL == store || CESK_STORE_ADDR_NULL == addr) 
	{
		LOG_ERROR("invailid argument");
		return NULL;
	}
	uint32_t b_idx = addr / CESK_STORE_BLOCK_NSLOTS;
	/* check the validity of the input store address */
	if(b_idx >= store->nblocks)
	{
		LOG_ERROR("invalid store address, out of memory(requesting block_no = %d in the store with %d blocks)", 
				  b_idx, store->nblocks);
		return NULL;
	}
	cesk_store_block_t* block = store->blocks[b_idx];
	/* check the refcount, so that future modification won't affect other store */
	if(block->refcnt > 1)
	{
		/* used by more than one store? copy it */
		LOG_DEBUG("%d stores are using this block, copy before change", block->refcnt);
		cesk_store_block_t* newblock = _cesk_store_block_duplicate(block);
		if(NULL == newblock)
		{
			LOG_ERROR("can not copy the block");
			return NULL;
		}
		newblock->refcnt = 1;
		block->refcnt --;   /* this is block-store ref count */
		store->blocks[b_idx] = newblock;
		block = newblock;
	}
	return block;
}
cesk_store_t* cesk_store_empty_store()
{
   cesk_store_t* ret = (cesk_store_t*)malloc(sizeof(cesk_store_t));
   if(NULL == ret)
   {
	   LOG_ERROR("can not allocate memory for empty store");
	   return NULL;
   }
   ret->nblocks = 0;
   ret->num_ent = 0;
   ret->hashcode = CESK_STORE_EMPTY_HASH;
   ret->alloc_tab = NULL;
   ret->blocks = NULL;
   return ret;
}
int cesk_store_set_alloc_table(cesk_store_t* store, cesk_alloctab_t* table)
{
	if(NULL == store)
	{
		LOG_ERROR("invalid argument");
		return -1;
	}
	store->alloc_tab = table;
	if(NULL != table) 
	{
		store->alloc_token = cesk_alloctab_get_token(table);
		LOG_DEBUG("allocate new token #%d", store->alloc_token);
	}
	return 0;
}
/**
 * @brief apply the allocation table to the block
 * @param store the result store
 * @param  base_addr the base address of this block
 * @param base_addr the base address of the block
 * @return < 0 : error
 **/
static inline int _cesk_store_apply_alloc_tab(cesk_store_t* store, uint32_t base_addr)
{
	cesk_store_block_t* blk = _cesk_store_getblock_rw(store, base_addr);
	if(NULL == blk)
	{
		LOG_ERROR("can not acquire writable pointer to the store block");
		return -1;
	}
	int ofs;

	for(ofs = 0; ofs < CESK_STORE_BLOCK_NSLOTS; ofs ++)
	{
		if(NULL == blk->slots[ofs].value || !cesk_value_get_reloc(blk->slots[ofs].value)) continue;
		/* if the value does contain a relocated address, apply the allocation table on that */
		LOG_DEBUG("object @0x%x contains relocated address, apply the relocation table on it", base_addr + ofs);
		/* acquire a writable pointer, get ready to write */
		cesk_value_t* value = cesk_store_get_rw(store, base_addr + ofs, 0);
		if(NULL == value)
		{
			LOG_ERROR("can not get writable pointer to the address");
			return -1;
		}
		/* clear the reloc flag of the vlaue */
		cesk_value_clear_reloc(value);
		cesk_set_iter_t iter;
		cesk_set_t* set;
		cesk_object_t* object;
		cesk_object_struct_t* object_base;
		uint32_t from_addr;
		uint32_t to_addr;
		int i,j;
		/* switch for each type */
		switch(value->type)
		{
			case CESK_TYPE_SET:
				set = value->pointer.set;
				if(NULL == cesk_set_iter(set, &iter))
				{
					LOG_ERROR("can not acquire set iterator");
					return -1;
				}
				for(;(from_addr = cesk_set_iter_next(&iter)) != CESK_STORE_ADDR_NULL;)
					if(CESK_STORE_ADDR_IS_RELOC(from_addr))
					{
						to_addr = cesk_alloctab_query(store->alloc_tab, store, from_addr);
						if(CESK_STORE_ADDR_NULL == to_addr)
						{
							LOG_WARNING("failed to query the allocation table for relocation address @0x%x", 
										to_addr);
							continue;
						}
						cesk_set_modify(set, from_addr, to_addr);
						LOG_DEBUG("apply map record @0x%x --> @0x%x", from_addr, to_addr);
					}
				break;
			case CESK_TYPE_OBJECT:
				object = value->pointer.object;
				object_base = object->members;
				for(i = 0; i < object->depth; i ++)
				{
					if(object_base->built_in)
					{
						/*if(bci_class_apply_atable(object_base->bcidata, store,object_base->class.bci->class) < 0)
						{
							LOG_WARNING("failed to apply the allocation table for object %s", object_base->class.path->value);
						}*/
						uint32_t addr_list[1024];
						uint32_t offset = 0;
						for(;;)
						{
							/* first of all, read data from built-in class */
							int rc = bci_class_read(object_base->bcidata, offset, addr_list, sizeof(addr_list) / sizeof(addr_list[0]), object_base->class.bci->class);
							if(rc < 0)
							{
								LOG_WARNING("can not read the address list");
								break;
							}
							if(0 == rc) break;
							/* then try to translate it */
							int i;
							for(i = 0; i < rc; i ++)
							{
								if(CESK_STORE_ADDR_IS_RELOC(addr_list[i]))
								{
									uint32_t result = cesk_alloctab_query(store->alloc_tab, store, addr_list[i]);
									if(CESK_STORE_ADDR_NULL == result)
									{
										LOG_WARNING("failed to query the allocation table for relocated address" PRSAddr, addr_list[i]);
										continue;
									}
									addr_list[i] = result;
								}
							}
							/* fianlly write back to the object */
							if(bci_class_write(object_base->bcidata, offset, addr_list, rc, object_base->class.bci->class) < 0)
							{
								LOG_WARNING("failed to write the modified data to object");
								continue;
							}
							offset += rc;
						}
					}
					else
					{
						for(j = 0; j < object_base->num_members; j ++)
						{
							from_addr = object_base->addrtab[j];
							if(CESK_STORE_ADDR_IS_RELOC(from_addr))
							{
								to_addr = cesk_alloctab_query(store->alloc_tab, store ,from_addr);
								if(CESK_STORE_ADDR_NULL == to_addr)
								{
									LOG_WARNING("failed to query the allocation table for relocated address @0x%x",
												to_addr);
									continue;
								}
								object_base->addrtab[j] = to_addr;
								LOG_DEBUG("apply map record @0x%x --> @0x%x", from_addr, to_addr);
							}
						}
					}
					CESK_OBJECT_STRUCT_ADVANCE(object_base);
				}
				break;
			default:
				LOG_ERROR("invalid value typecode(%u)", value->type);
				return -1;
		}
		/* release the write pointer */
		cesk_store_release_rw(store, base_addr + ofs);
	}
	
	if(blk->num_reloc != 0)
	{
		LOG_ERROR("opps, unexpected num_reloc after applying allocation table to store, something goes wrong!");
		return -1;
	}
	return 0;
}
int cesk_store_apply_alloctab(cesk_store_t* store)
{
	if(NULL == store->alloc_tab) return 0;   /* no allocation table availible, so just return success */
	uint32_t base_addr;
	int i;
	for(i = 0, base_addr = 0; i < store->nblocks; i ++, base_addr += CESK_STORE_BLOCK_NSLOTS)
	{
		if(store->blocks[i]->num_reloc > 0)
		{
			/* there's some relocated address here */
			if(_cesk_store_apply_alloc_tab(store, base_addr) < 0)
			{
				LOG_ERROR("can not apply allocation table to the store");
				return -1;
			}
		}
	}
	store->alloc_tab = NULL;
	return 0;
}
cesk_store_t* cesk_store_fork(const cesk_store_t* store)
{
	int i;
	size_t size = sizeof(cesk_store_t);
	size_t block_size = sizeof(cesk_store_block_t*) * store->nblocks;
	cesk_store_t* ret = (cesk_store_t*)malloc(size);
	if(NULL == ret)
	{
		LOG_ERROR("can not allocate memory for new store");
		return NULL;
	}
	/* what we should do is just duplicating the block table in the block */
	memcpy(ret, store, size);
	/* if there's some blocks here, allocate memory for it */
	if(ret->nblocks > 0)
	{
		cesk_store_block_t** blocks = (cesk_store_block_t**)malloc(block_size);
		memcpy(blocks, store->blocks, block_size);
		ret->blocks = blocks;
	}
	/* use the new copy of the block counter */
	uint32_t base_addr = 0;
	/* increase refrence counter of all blocks */
	for(i = 0; i < ret->nblocks; i ++, base_addr += CESK_STORE_BLOCK_NSLOTS)
		ret->blocks[i]->refcnt++;
	LOG_DEBUG("a store of %d entities is being forked, %zu bytes copied", ret->num_ent, size);
	return ret;
}
/**
 * @brief make the object address from a store address (might be relocated address)
 * @param store
 * @param addr the store address
 * @return the object address converted from the store address, CESK_STORE_ADDR_NULL means error
 **/
static inline uint32_t _cesk_store_make_object_address(const cesk_store_t* store, uint32_t addr)
{
	uint32_t ret = addr;
	if(CESK_STORE_ADDR_IS_RELOC(addr))
	{
		/* if this address is a relocated address */
		if(NULL == store->alloc_tab)
		{
			LOG_DEBUG("try to acquire a relocated address without an allocation table address=@0x%x", addr);
			return CESK_STORE_ADDR_NULL;
		}
		ret = cesk_alloctab_query(store->alloc_tab, store, addr);
		if(CESK_STORE_ADDR_NULL == ret)
		{
			LOG_DEBUG("there's no record for this store at relocated address @0x%x", addr);
			return CESK_STORE_ADDR_NULL;
		}
	}
	return ret;
}
cesk_value_const_t* cesk_store_get_ro(const cesk_store_t* store, uint32_t addr)
{
	if(CESK_STORE_ADDR_NULL == (addr = _cesk_store_make_object_address(store, addr))) return NULL;
	uint32_t block_idx = addr / CESK_STORE_BLOCK_NSLOTS;
	uint32_t offset    = addr % CESK_STORE_BLOCK_NSLOTS;
	if(block_idx >= store->nblocks) 
	{
		LOG_ERROR("invalid address out of space");
		return NULL;
	}
	const cesk_store_block_t* block = store->blocks[block_idx];
	if(NULL == block)
	{
		LOG_ERROR("opps, it should not happen");
		return NULL;
	}
	return (cesk_value_const_t*)block->slots[offset].value;
}
int cesk_store_get_reuse(const cesk_store_t* store, uint32_t addr)
{
	if(CESK_STORE_ADDR_NULL == (addr = _cesk_store_make_object_address(store, addr))) return -1;
	uint32_t block_idx = addr / CESK_STORE_BLOCK_NSLOTS;
	uint32_t offset    = addr % CESK_STORE_BLOCK_NSLOTS;
	if(block_idx >= store->nblocks)
	{
		LOG_ERROR("out of memory");
		return -1;
	}
	const cesk_store_block_t* block = store->blocks[block_idx];
	if(NULL == block)
	{
		LOG_ERROR("ooops, this should not happen");
		return -1;
	}
	return block->slots[offset].reuse;
}
int cesk_store_set_reuse(cesk_store_t* store, uint32_t addr)
{
	if(CESK_STORE_ADDR_NULL == (addr = _cesk_store_make_object_address(store, addr))) return -1;
	uint32_t block_idx = addr / CESK_STORE_BLOCK_NSLOTS;
	uint32_t offset = addr % CESK_STORE_BLOCK_NSLOTS;
	if(block_idx >= store->nblocks)
	{
		LOG_ERROR("out of memory");
		return -1;
	}
	cesk_store_block_t* block = _cesk_store_getblock_rw(store, addr);
	if(block->slots[offset].value->write_count == 0)
		store->hashcode ^= HASH_INC(addr, block->slots[offset].value, block->slots[offset].reuse);
	if(NULL == block)
	{
		LOG_ERROR("what's wrong?");
		return -1;
	}
	block->slots[offset].reuse = 1;
	if(block->slots[offset].value->write_count == 0)
		store->hashcode ^= HASH_INC(addr, block->slots[offset].value, block->slots[offset].reuse);
	return 0;
}
int cesk_store_clear_reuse(cesk_store_t* store, uint32_t addr)
{
	if(CESK_STORE_ADDR_NULL == (addr = _cesk_store_make_object_address(store, addr))) return -1;
	uint32_t block_idx = addr / CESK_STORE_BLOCK_NSLOTS;
	uint32_t offset = addr % CESK_STORE_BLOCK_NSLOTS;
	if(block_idx >= store->nblocks)
	{
		LOG_ERROR("out of memory");
		return -1;
	}
	cesk_store_block_t* block = _cesk_store_getblock_rw(store, addr);
	store->hashcode ^= HASH_INC(addr, block->slots[offset].value, block->slots[offset].reuse);
	if(NULL == block)
	{
		LOG_ERROR("can not acquire an writable pointer to the block");
		return -1;
	}
	block->slots[offset].reuse = 0;
	store->hashcode ^= HASH_INC(addr, block->slots[offset].value, block->slots[offset].reuse);
	return 0;
}
cesk_value_t* cesk_store_get_rw(cesk_store_t* store, uint32_t addr, int noval)
{
	if(CESK_STORE_ADDR_NULL == (addr = _cesk_store_make_object_address(store, addr))) return NULL;
	uint32_t offset    = addr % CESK_STORE_BLOCK_NSLOTS;
	cesk_store_block_t* block = _cesk_store_getblock_rw(store, addr);
	if(NULL == block)
	{
		LOG_ERROR("failed to get block #%"PRIu32, (uint32_t)(addr/CESK_STORE_BLOCK_NSLOTS));
		return NULL;
	}
	cesk_value_t* val = block->slots[offset].value;
	if(NULL == val) return NULL;
	if(val->refcnt > 1 && noval == 0)
	{
		LOG_DEBUG("this value is refered by other frame block, so fork it first");
		cesk_value_t* newval = cesk_value_fork(val);
		
		if(NULL == newval)
		{
			LOG_ERROR("error during copy the value");
			return NULL;
		}

		block->slots[offset].value = newval;

		/* maintain the value-block ref count */
		cesk_value_decref(val);
		cesk_value_incref(newval);

		val = newval;
	}
	/* when a rw pointer is auquired, the hashcode is ready to update.
	 * After finish updating, you should call the function release the 
	 * value and update the hashcode */
	store->hashcode ^= HASH_INC(addr, val, block->slots[offset].reuse);
	/* decrease the reloc num first, and we are going to increase it back */
	if(val->reloc) block->num_reloc --;
	if(val->write_count > 15) 
		LOG_WARNING("too many write pointer acquired, which up to 15 at store address "PRSAddr, addr);
	else
		val->write_count ++;
	return val;
}
void cesk_store_release_rw(cesk_store_t* store, uint32_t addr)
{
	if(CESK_STORE_ADDR_NULL == (addr = _cesk_store_make_object_address(store, addr))) return;
	uint32_t block_idx = addr / CESK_STORE_BLOCK_NSLOTS;
	uint32_t offset    = addr % CESK_STORE_BLOCK_NSLOTS;
	if(block_idx >= store->nblocks) 
	{
		LOG_ERROR("invalid address "PRSAddr" out of space", addr);
		return;
	}
	cesk_store_block_t* block = _cesk_store_getblock_rw(store, addr);
	if(NULL == block)
	{
		LOG_ERROR("opps, can not get the writable pointer to store address"PRSAddr, addr);
		return;
	}
	cesk_value_t* val = block->slots[offset].value;
	if(val->write_count == 0)
	{
		LOG_WARNING("there's no writable pointer accociated to this status");
		return;
	}
	val->write_count --;
	/* flush the modification result to the store */
	if(0 == val->write_count)
	{
		/* update the relocation bit */
		if(val->reloc) block->num_reloc ++;
		/* update the hashcode */
		store->hashcode ^= HASH_INC(addr, val, block->slots[offset].reuse);
	}
}
/* just for debug purpose */
hashval_t cesk_store_compute_hashcode(const cesk_store_t* store)
{
	uint32_t ret = CESK_STORE_EMPTY_HASH;
	int i,j;
	for(i = 0; i < store->nblocks; i ++)
	{
		for(j = 0; j < CESK_STORE_BLOCK_NSLOTS; j ++)
		{
			if(NULL != store->blocks[i]->slots[j].value && store->blocks[i]->slots[j].value->write_count == 0)
			{
				uint32_t addr = i * CESK_STORE_BLOCK_NSLOTS + j;
				ret ^= HASH_CMP(addr, store->blocks[i]->slots[j].value, store->blocks[i]->slots[j].reuse);
			}
		}
	}
	return ret;
}
/**
 * @note caller should update the reuse flag manually 
 **/
uint32_t cesk_store_allocate(cesk_store_t* store, const cesk_alloc_param_t* param)
{
	uint32_t  init_slot = cesk_alloc_param_hash(param)  % CESK_STORE_BLOCK_NSLOTS;
	uint32_t  slot = init_slot;
	/* here we perform a quadratic probing inside each block
	 * But we do not jump more than 5 times in one block
	 * After tried 5 jumps, the allocator should allocate a 
	 * new store block anyway.
	 */
	int empty_block = -1;
	int empty_offset = -1;
	int equal_block = -1;
	int equal_offset = -1;
	int attempt;
	for(attempt = 0; attempt < CESK_STORE_ALLOC_ATTEMPT; attempt ++)
	{
		int block;
		LOG_DEBUG("attempt #%d : slot "PRSAddr" for instruction 0x%x", attempt, slot, param->inst);
		for(block = 0; block < store->nblocks; block ++)
		{
			if(!store->blocks[block]->slots[slot].inuse && empty_offset == -1)
			{
				LOG_DEBUG("find an empty slot @(block = 0x%x, offset = 0x%x)", block, slot);
				empty_block = block;
				empty_offset = slot;
			}
			if(store->blocks[block]->slots[slot].inuse && 
			   cesk_alloc_param_equal(param, &store->blocks[block]->slots[slot].param))
			{
				LOG_DEBUG("find the equal slot @(block = 0x%x, offset = 0x%x)", block, slot);
				equal_block = block;
				equal_offset = slot;
			}
			if(equal_offset != -1) break;
		}
		if(equal_offset != -1) break;
		slot = (slot * slot * MH_MULTIPLY + 100007 * slot + 634567) % CESK_STORE_BLOCK_NSLOTS;
	}
	/* After look for empty slot / equal slot */
	if(empty_offset == -1 && equal_offset == -1)
	{
		/* no empty slot, no equal slot, allocate a new block */
		LOG_DEBUG("can not allocate a store entry for this object,"
				  "allocate a new block. current_size = %d, num_ent = %d", 
				  store->nblocks, store->num_ent);
		cesk_store_block_t** blocks;
		if(store->nblocks > 0)
			blocks = (cesk_store_block_t**)realloc(store->blocks , sizeof(cesk_store_block_t*) * (store->nblocks + 1));
		else
			blocks = (cesk_store_block_t**)malloc(sizeof(cesk_store_block_t*));
		//store = realloc(store, sizeof(cesk_store_t) + sizeof(cesk_store_block_t*) * (store->nblocks + 1));
		if(NULL == blocks)
		{
			LOG_ERROR("can not increase the size of store");
			return CESK_STORE_ADDR_NULL;
		}
		store->blocks = blocks;
		//(*p_store) = store;
		store->blocks[store->nblocks] = (cesk_store_block_t*)malloc(CESK_STORE_BLOCK_SIZE);
		if(NULL ==  store->blocks[store->nblocks])
		{
			LOG_ERROR("can not allocate a new page for the block");
			return CESK_STORE_ADDR_NULL;
		}
		memset(store->blocks[store->nblocks], 0, CESK_STORE_BLOCK_SIZE);
		store->blocks[store->nblocks]->refcnt ++;
		empty_block = store->nblocks ++;
		empty_offset = init_slot;   /* use the init_slot, so that we can locate it faster */
	}
	/* if there's no equal entry */
	if(equal_offset == -1)
	{
		if(empty_offset != -1)
		{
			LOG_DEBUG("allocate 0x%"PRIx32" (block=0x%x, offset = 0x%x) for instruction 0x%x", 
						(uint32_t)(empty_block * CESK_STORE_BLOCK_NSLOTS + empty_offset), 
						empty_block, empty_offset, param->inst);
			store->blocks[empty_block]->slots[empty_offset].param = *param;
			store->blocks[empty_block]->slots[empty_offset].reuse = 0;
			store->blocks[empty_block]->slots[empty_offset].inuse = 1;
			return empty_block * CESK_STORE_BLOCK_NSLOTS + empty_offset;
		}
		else
		{
			LOG_ERROR("neither empty slot or equal entry has been found in the store, allocation failed");
			return CESK_STORE_ADDR_NULL;
		}
	}
	else
	{
		/* some equal entry */
		LOG_DEBUG("reuse %"PRIx32" (block = 0x%x, offset = 0x%x for instruction 0x%x",
				  (uint32_t)(equal_block * CESK_STORE_BLOCK_NSLOTS + equal_offset), equal_block, equal_offset, param->inst);
		/* do not set the reuse bit here */
		//store->blocks[equal_block]->slots[equal_offset].reuse = 1;   
		return equal_block * CESK_STORE_BLOCK_NSLOTS  + equal_offset;
	}
}
int cesk_store_attach(cesk_store_t* store, uint32_t addr, cesk_value_t* value)
{
	if(NULL == store || CESK_STORE_ADDR_NULL == addr)
	{
		LOG_ERROR("invalid arguments");
		return -1;
	}
	if(CESK_STORE_ADDR_NULL == (addr = _cesk_store_make_object_address(store, addr))) return -1;
	uint32_t block = addr / CESK_STORE_BLOCK_NSLOTS;
	uint32_t offset = addr % CESK_STORE_BLOCK_NSLOTS;
	if(block >= store->nblocks)
	{
		LOG_ERROR("out of memory");
		return -1;
	}
	if(value == store->blocks[block]->slots[offset].value)
	{
		LOG_TRACE("value is already attached to this address");
		return 0;
	}
	/* just acquire a writable pointer of this block */
	cesk_store_block_t* block_rw = _cesk_store_getblock_rw(store, addr);
	/* Assign an empty slot to a non-empty value means we add some new value to store */
	if(block_rw->slots[offset].value == NULL && value != NULL) 
	{
		block_rw->num_ent ++;
		store->num_ent ++;
	}
	/* On the other hand, if we assign a non-empty slot with a empty value, that means we want to
	 * clean the value */
	else if(block_rw->slots[offset].value != NULL && value == NULL)
	{
		block_rw->num_ent --;
		store->num_ent --;
		block_rw->slots[offset].inuse = 0;
	}
	/* And we should swipe the old value out, but we can not affect the ref count, because
	 * No matter what the slot contains, the ref count does not depends on the value */
	if(NULL != block_rw->slots[offset].value)
		_cesk_store_swipe(store, block_rw, addr);
	if(value)
	{
		/* reference to new value */
		cesk_value_incref(value);
		value->write_count = 1;
		/* we do not update new hash code here, that means we should use cesk_store_release_rw function
		 * After we finish modifiying the store */
	}
	block_rw->slots[offset].value = value;
	block_rw->slots[offset].reuse = 0;  /* attach to a object, all previous object is lost */
	return 0;
}
void cesk_store_free(cesk_store_t* store)
{
	int i;
	if(NULL == store) return;
	for(i = 0; i < store->nblocks; i ++)
	{
		if(store->blocks[i]->refcnt > 0)
		store->blocks[i]->refcnt --;
		if(store->blocks[i]->refcnt == 0)
		{
			int j;
			for(j = 0; j < CESK_STORE_BLOCK_NSLOTS; j ++)
				if(store->blocks[i]->slots[j].value != NULL)
					cesk_value_decref(store->blocks[i]->slots[j].value);
			free(store->blocks[i]);
		}
	}
	if(store->nblocks > 0) free(store->blocks);
	free(store);
}
/**
 * @note this is actually increase the slot refcnt(which used for gabage collection)
 **/
int cesk_store_incref(cesk_store_t* store, uint32_t addr)
{
	if(CESK_STORE_ADDR_NULL == (addr = _cesk_store_make_object_address(store, addr))) return -1;
	if(CESK_STORE_ADDR_IS_CONST(addr))
	{
		return 0;
	}
	uint32_t b_ofs = addr % CESK_STORE_BLOCK_NSLOTS;
	cesk_store_block_t* block = _cesk_store_getblock_rw(store, addr);
	if(NULL == block)
		return -1;
	if(block->slots[b_ofs].value != NULL)
	{
		return ++block->slots[b_ofs].refcnt;
	}
	else
	{
		LOG_ERROR("the object "PRSAddr" is alread dead", addr);
		return -1;
	}
}
/**
 * @note this is actually increase the slot refcnt(which used for gabage collection)
 **/
int cesk_store_decref(cesk_store_t* store, uint32_t addr)
{
	if(CESK_STORE_ADDR_NULL == (addr = _cesk_store_make_object_address(store, addr))) return -1;
	if(CESK_STORE_ADDR_IS_CONST(addr))
	{
		return 0;
	}
	uint32_t ofs = addr % CESK_STORE_BLOCK_NSLOTS;
	cesk_store_block_t* block = _cesk_store_getblock_rw(store, addr);
	if(NULL == block)
	{
		LOG_ERROR("can not acquire writable pointer to block");
		return -1;
	}

	if(block->slots[ofs].value == NULL)
	{
		LOG_DEBUG("the value is empty");
		return 0;
	}

	/* decrease the counter */
	if(block->slots[ofs].refcnt > 0) 
		block->slots[ofs].refcnt --;
	else
		LOG_WARNING("found an living object with 0 refcnt at address %x", addr);

	if(0 == block->slots[ofs].refcnt)
	{
		LOG_TRACE("value @0x%x is dead, swipe it out", addr);
		_cesk_store_swipe(store, block, addr);
		block->num_ent --;
		store->num_ent --;
	}
	return block->slots[ofs].refcnt;
}


int cesk_store_equal(const cesk_store_t* first, const cesk_store_t* second)
{
	if(NULL == first || NULL == second) return first != second;
	if(first->nblocks != second->nblocks) return 0;
	if(cesk_store_hashcode(first) != cesk_store_hashcode(second)) return 0;
	int i;
	for(i = 0; i < first->nblocks; i ++)
	{
		int j;
		for(j = 0; j < CESK_STORE_BLOCK_NSLOTS; j ++)
		if(cesk_value_equal(first->blocks[i]->slots[j].value, second->blocks[i]->slots[j].value) == 0)
			return 0;
	}
	return 1;
}
uint32_t cesk_store_const_addr_from_operand(const dalvik_operand_t* operand)
{
	if(NULL == operand) return CESK_STORE_ADDR_NULL;
	if(!operand->header.info.is_const)
	{
		LOG_ERROR("can not create a value from a non-constant operand");
		return CESK_STORE_ADDR_NULL;
	}
	int intval = 0;
	switch(operand->header.info.type)
	{
		case DVM_OPERAND_TYPE_LONG:
		intval = operand->payload.int64;
		goto num;
		case DVM_OPERAND_TYPE_SHORT:
		intval = operand->payload.int16;
		goto num;
		case DVM_OPERAND_TYPE_INT:
		intval = operand->payload.int32;
		goto num;
		case DVM_OPERAND_TYPE_BYTE:
		intval = operand->payload.int8;
		goto num;
		case DVM_OPERAND_TYPE_DOUBLE:
		if(operand->payload.real64 < -1e-20)
			intval = -1;
		else if(operand->payload.real64 > 1e-20)
			intval = 1;
		else 
			intval = 0;
		goto num;
		case DVM_OPERAND_TYPE_FLOAT:
		if(operand->payload.real32 < -1e-20)
			intval = -1;
		else if(operand->payload.real32 > 1e-20)
			intval = 1;
		else 
			intval = 0;
		goto num;
		case DVM_OPERAND_TYPE_CHAR:
		intval = operand->payload.int8;
num:
		if(intval > 0)
			return CESK_STORE_ADDR_POS;
		else if(intval < 0)
			return CESK_STORE_ADDR_NEG;
		else 
			return CESK_STORE_ADDR_ZERO;
		case DALVIK_TYPECODE_BOOLEAN:
		if(operand->payload.uint8)
			return CESK_STORE_ADDR_TRUE;
		else
			return CESK_STORE_ADDR_FALSE;
		break;
		default:
		LOG_ERROR("can not create value for the type");
			return CESK_STORE_ADDR_NULL;
	}
	return CESK_STORE_ADDR_NULL;
}
uint32_t cesk_store_get_refcnt(const cesk_store_t* store, uint32_t addr)
{
	if(CESK_STORE_ADDR_NULL == (addr = _cesk_store_make_object_address(store, addr))) return -1;  /* TODO: is it OK to return -1? */
	uint32_t idx = addr / CESK_STORE_BLOCK_NSLOTS;
	uint32_t ofs = addr % CESK_STORE_BLOCK_NSLOTS;
	if(store->nblocks <= idx) return 0;
	return store->blocks[idx]->slots[ofs].refcnt;
}
int cesk_store_clear_refcnt(cesk_store_t* store, uint32_t addr)
{
	if(CESK_STORE_ADDR_NULL == (addr = _cesk_store_make_object_address(store, addr))) return -1;
	uint32_t ofs = addr % CESK_STORE_BLOCK_NSLOTS;
	cesk_store_block_t* block = _cesk_store_getblock_rw(store, addr);
	if(NULL == block)
	{
		LOG_ERROR("can not get a writable pointer to the block");
		return -1;
	}
	block->slots[ofs].refcnt = 0;
	return 0;
}
#define __PR(fmt, args...) do{\
	int pret = snprintf(p, buf + sz - p, fmt, ##args);\
	if(pret > buf + sz - p) pret = buf + sz - p;\
	p += pret;\
}while(0)
const char* cesk_store_to_string(const cesk_store_t* store, char* buf, size_t sz)
{
	static char _buf[1024];
	if(NULL == buf)
	{
		buf = _buf;
		sz = sizeof(_buf);
	}
	char* p = buf;
	uint32_t blkidx;
	for(blkidx = 0; blkidx < store->nblocks; blkidx ++)
	{
		const cesk_store_block_t* blk = store->blocks[blkidx];
		uint32_t ofs;
		for(ofs = 0; ofs < CESK_STORE_BLOCK_NSLOTS; ofs ++)
		{
			if(blk->slots[ofs].value == NULL) continue;
			uint32_t addr = blkidx * CESK_STORE_BLOCK_NSLOTS + ofs;
			uint32_t reloc_addr = cesk_alloctab_query(store->alloc_tab, store, addr);
			if(reloc_addr != CESK_STORE_ADDR_NULL)
				__PR("(["PRSAddr" --> "PRSAddr"] %s) ", reloc_addr, addr, cesk_value_to_string(blk->slots[ofs].value, NULL, 0));
			else
				__PR("("PRSAddr": %s)", addr, cesk_value_to_string(blk->slots[ofs].value, NULL, 0));
		}
	}
	return buf;
}
#undef __PR
void cesk_store_print_debug(const cesk_store_t* store)
#if LOG_LEVEL >= 6
{
	uint32_t blkidx;
	for(blkidx = 0; blkidx < store->nblocks; blkidx ++)
	{
		const cesk_store_block_t* blk = store->blocks[blkidx];
		uint32_t ofs;
		for(ofs = 0; ofs < CESK_STORE_BLOCK_NSLOTS; ofs ++)
		{
			if(blk->slots[ofs].value == NULL) continue;
			uint32_t addr = blkidx * CESK_STORE_BLOCK_NSLOTS + ofs;
			uint32_t reloc_addr = cesk_alloctab_query(store->alloc_tab, store, addr);
			if(reloc_addr != CESK_STORE_ADDR_NULL)
				LOG_DEBUG("\t"PRSAddr"("PRSAddr")\t%s", reloc_addr, addr,cesk_value_to_string(blk->slots[ofs].value, NULL, 0));
			else
				LOG_DEBUG("\t"PRSAddr"\t%s", addr, cesk_value_to_string(blk->slots[ofs].value, NULL, 0));
		}
	}
}
#else
{}
#endif
int cesk_store_compact_store(cesk_store_t* store)
{
	if(NULL == store)
	{
		LOG_ERROR("invalid argument");
		return -1;
	}
	for(; store->nblocks > 0 && 0 == store->blocks[store->nblocks - 1]->num_ent; store->nblocks --)
	{
		cesk_store_block_t* blk = store->blocks[store->nblocks - 1];
		/* because this block is empty, so that we do not need to decref for the values in the block */
		if(-- blk->refcnt == 0)  free(blk);
	}
	/* if this store is actually empty, we should free the blocks array */
	if(0 == store->nblocks && store->blocks)
	{
		free(store->blocks);
		store->blocks = NULL;
	}
	LOG_DEBUG("the number of blocks after compact is %d", store->nblocks);
	return 0;
}
