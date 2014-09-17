/**
 * @file cesk_object.c
 * @brief implementation of object instance
 **/

#include <string.h>

#include <log.h>

#include <dalvik/dalvik_class.h>
#include <dalvik/dalvik_method.h>
#include <dalvik/dalvik_field.h>
#include <dalvik/dalvik_memberdict.h>

#include <cesk/cesk_set.h>
#include <cesk/cesk_object.h>

#include <bci/bci_nametab.h>
cesk_object_t* cesk_object_new(const char* classpath)
{
	int field_count = 0;
	int class_count = 0;
	size_t builtin_size = 0;
	int nbci = 0;
	const bci_class_wrap_t* bci_class[CESK_OBJECT_MAX_BUILTIN_CLASSES];
	const dalvik_class_t* classes[CESK_OBJECT_MAX_USER_DEFINED_CLASSES]; 
	/* find classes inheritation relationship to determine its memory layout */
	for(;NULL != classpath;)
	{
		LOG_DEBUG("try to find class %s", classpath);
		const dalvik_class_t* target_class = dalvik_memberdict_get_class(classpath);
		/* not found, try to find built-in class */
		if(NULL == target_class)
		{
			const bci_class_wrap_t* class_wrap = bci_nametab_get_class(classpath);
			/* we found a built-in class */
			if(NULL != class_wrap)
			{
				LOG_DEBUG("found built-in class %s", classpath);
				builtin_size += class_wrap->class->size + sizeof(cesk_object_struct_t);
				bci_class[nbci ++] = class_wrap;
				if(nbci >= CESK_OBJECT_MAX_BUILTIN_CLASSES)
				{
					LOG_ERROR("too many built-in class in the class inheritate tree"
					          "(try to adjust CESK_OBJECT_MAX_BUILTIN_CLASSES)"
							  "which currently is %u", CESK_OBJECT_MAX_BUILTIN_CLASSES);
					return NULL;
				}
				classpath = class_wrap->class->super;
				continue;
			}
			/* not a built-in class, so we don't know the type of this class. */
			LOG_WARNING("can not find class %s", classpath);
			break;
		}
		int i;
		/* we should count how many members there */
		for(i = 0; target_class->members[i]; i ++) field_count ++;
		classes[class_count ++] = target_class;
		if(class_count >= CESK_OBJECT_MAX_USER_DEFINED_CLASSES)
		{
			LOG_ERROR("a class with inherent from more than 1024 classes? are you kidding me?"
			          "(try to adjust CESK_OBJECT_MAX_USER_DEFINED_CLASSES which currently is"
					  " %u", CESK_OBJECT_MAX_USER_DEFINED_CLASSES);
			return NULL;
		}
		LOG_DEBUG("found class %s at %p", classpath, target_class);
		classpath = target_class->super;
	}
	/* compute the size required for this instance */
	size_t size = sizeof(cesk_object_t) +                   	   /* header */
				  sizeof(cesk_object_struct_t) * class_count +     /* class header */
				  sizeof(cesk_set_t*) * field_count +              /* fields */
				  builtin_size;                                    /* memory for built-ins */         
	
	/* okay, create the new object */
	cesk_object_t* object = (cesk_object_t*)malloc(size);
	if(NULL == object)
	{
		LOG_ERROR("can not allocate memory for new object %s", classes[0]->path);
		return NULL;
	}
	
	cesk_object_struct_t* base = object->members; 
	object->builtin = NULL;

	/* initialize the user-defined class */
	int i;
	for(i = 0; i < class_count; i ++)
	{
		/* if this is a user defined class */
		int j;
		for(j = 0; classes[i]->members[j]; j ++)
		{
			const dalvik_field_t* field = dalvik_memberdict_get_field(classes[i]->path, classes[i]->members[j]); 
			if(NULL == field)
			{
				LOG_WARNING("Can not find field %s/%s, skip", classes[i]->path, classes[i]->members[j]);
				continue;
			}
			base->addrtab[field->offset] = CESK_STORE_ADDR_NULL;  
			LOG_DEBUG("Create new filed %s/%s for class %s at offset %d", classes[i]->path, 
																		 classes[i]->members[j], 
																		 classes[0]->path, 
																		 field->offset);
		}
		base->built_in = 0;
		base->class.udef = classes[i];
		base->num_members = j;
		CESK_OBJECT_STRUCT_ADVANCE(base);
	}
	/* We do not initalize the built-in class here, we do it when the field object of user defined class assigned*/
	for(i = 0; i < nbci; i ++)
	{
		base->built_in = 1;
		base->class.bci = bci_class[i];
		base->num_members = bci_class[i]->class->size;
		if(NULL == object->builtin) object->builtin = base;
		if(base->num_members > 0) base->bcidata[0] = 0; /* just initialize the first byte */
		CESK_OBJECT_STRUCT_ADVANCE(base);
	}
	object->depth = class_count + nbci;
	object->nbuiltin = nbci;
	object->size = size;
	object->tags = tag_set_empty();
	return object;
}
uint32_t* cesk_object_get(
		cesk_object_t* object, 
		const char* classpath, 
		const char* field_name, 
		const bci_class_t** p_bci_class,
		void** p_bci_data)
{
	if(NULL == object    ||
	   NULL == classpath ||
	   NULL == field_name)
	{
		LOG_ERROR("invalid arguments");
		return NULL;
	}
	if(NULL != p_bci_class) *p_bci_class = NULL;
	if(NULL != p_bci_data) *p_bci_data = NULL;

	int i;
	cesk_object_struct_t* this = object->members;
	for(i = 0; i < object->depth; i ++)
	{
		if(this->class.path->value == classpath) break;
		/* move to next object struct */
		CESK_OBJECT_STRUCT_ADVANCE(this);
	}
	if(object->depth == i)
	{
		LOG_WARNING("I can't find field named %s/%s in instance object of %s", 
					classpath, 
					field_name, 
					cesk_object_classpath(object));
		return NULL;
	}
	/* because the instruction only provides an lower bound, so that we have to go all the way up */
	for(; i < object->depth; i ++)
	{
		if(this->built_in)
		{
			if(NULL == p_bci_class || NULL == p_bci_data)
			{
				LOG_ERROR("built-in class does not support this interface");
				return NULL;
			}
			else
			{
				if(NULL != p_bci_class) *p_bci_class = this->class.bci->class;
				if(NULL != p_bci_data) *p_bci_data = this->bcidata;
				return NULL;
			}
		}
		const dalvik_field_t* field = dalvik_memberdict_get_field(classpath, field_name);
		if(NULL != field) return this->addrtab + field->offset;
		CESK_OBJECT_STRUCT_ADVANCE(this);
		classpath = this->class.path->value;
	}

	LOG_WARNING("No field named %s/%s", classpath, field_name);
	return NULL;
}
void cesk_object_free(cesk_object_t* object)
{
	if(NULL == object) return;
	/* before we free the object, we should call finailize function for each BCI instance */
	cesk_object_struct_t* this = object->builtin;
	int i;
	for(i = 0; i < object->nbuiltin; i ++)
	{
		if(!this->built_in)
		{
			LOG_WARNING("an built-in class %s interitate from an user-defined class %s, this is impossible",
					  cesk_object_classpath(object), this->class.path->value);
		}
		else if(bci_class_finalize(this->bcidata, this->class.bci->class) < 0)
		{
			LOG_WARNING("the finalization function of class %s returns an error", 
			          cesk_object_classpath(object));
		}
		CESK_OBJECT_STRUCT_ADVANCE(this);
	}
	free(object);   /* the object is just an array */
}
/**
 * @note this function is actually a simple copy function, Copy-On-Write machenism is 
 *       acomplished by cesk_value_t type
 **/
cesk_object_t* cesk_object_duplicate(const cesk_object_t* object)
{
	LOG_TRACE("copy object %s@%p", cesk_object_classpath(object), object);
	size_t objsize = object->size;
	cesk_object_t* newobj = (cesk_object_t*)malloc(objsize);
	if(NULL == newobj) 
	{
		LOG_ERROR("can not allocate memory for the new object");
		return NULL;
	}
	memcpy(newobj, object, objsize);
	newobj->builtin = (cesk_object_struct_t*)(((char *)newobj)  + CESK_OBJECT_FIELD_OFS(object, object->builtin));
	newobj->tags = tag_set_fork(object->tags);

	/* we have a builtin class struct section , duplicate it */
	const cesk_object_struct_t* this = object->builtin;
	cesk_object_struct_t* that = newobj->builtin;

	int i;
	for(i = 0; i < object->nbuiltin; i ++)
	{
		if(!this->built_in)
		{
			LOG_ERROR("an built-in class %s interitate from an user-defined class %s, this is impossible",
					  cesk_object_classpath(object), this->class.path->value);
			free(newobj);
			return NULL;
		}
		else if(bci_class_duplicate(this->bcidata, that->bcidata, this->class.bci->class) < 0)
		{
			LOG_ERROR("failed to initalize builtin class %s", object->builtin->class.path->value);
			free(newobj);
			return NULL;
		}
		CESK_OBJECT_STRUCT_ADVANCE(this);
		CESK_OBJECT_STRUCT_ADVANCE(that);
	}
	return newobj;
}

hashval_t cesk_object_hashcode(const cesk_object_t* object)
{
	hashval_t  hash = ((uintptr_t)object->members[0].class.path) & ~(hashval_t)0;    /* We also consider the type of the object */
	
	int i;
	const cesk_object_struct_t* this = object->members;
	uint32_t mul = MH_MULTIPLY;
	for(i = 0; i < object->depth; i ++)
	{
		int j;
		mul ^= (uintptr_t)this->class.path;
		if(this->built_in)
		{
			hashval_t h = 0x73658217ul * bci_class_hashcode(this->bcidata, this->class.bci->class) * MH_MULTIPLY ;
			hash ^= (h * h);
		}
		else
		{
			for(j = 0; j < this->num_members; j ++)
			{
				hashval_t k = this->addrtab[j] * mul;
				mul *= MH_MULTIPLY;
				hash ^= k;
				hash = (hash << 16) | (hash >> 16);
			}
		}
		CESK_OBJECT_STRUCT_ADVANCE(this);
	}
	hashval_t ts_hash = tag_set_hashcode(object->tags);
	return hash ^ (ts_hash * ts_hash + MH_MULTIPLY * ts_hash);
}
hashval_t cesk_object_compute_hashcode(const cesk_object_t* object)
{
	/* the object hash itself is non-incremental style, so call the hashcode function directly */
	return cesk_object_hashcode(object);
}
int cesk_object_equal(const cesk_object_t* first, const cesk_object_t* second)
{
	if(NULL == first || NULL == second) return first == second;
	if(first->members[0].class.path != second->members[0].class.path) return 0;
	const cesk_object_struct_t* this = first->members;
	const cesk_object_struct_t* that = second->members;

	/* if the class path is the same, we assume that two object has the same depth  */
	int i;
	if(first->depth != second->depth)
	{
		LOG_WARNING("two object build from the same class %s has defferent inherent depth", cesk_object_classpath(first));
		return 0;
	}
	/* compare the tag set first */
	if(tag_set_equal(first->tags, second->tags) == 0) return 0;
	/* do actuall comparasion */
	for(i = 0; i < first->depth; i ++)
	{
		int j;
		if(this->built_in != that->built_in)
		{
			LOG_WARNING("two object build from the same class %s has different memory structure", cesk_object_classpath(first));
			LOG_WARNING("first class is %s class %s, but the second one is %s class %s", 
							(this->built_in?"built-in":"user defined"), 
							cesk_object_classpath(first),
							(that->built_in?"built-in":"user defined"), 
							cesk_object_classpath(second));
			return 0;
		}
		if(this->built_in)
		{
			int rc = bci_class_equal(this->bcidata, that->bcidata, this->class.bci->class);
			if(rc < 0)
			{
				LOG_WARNING("failed to compare this two object %p and %p", first, second);
				return -1;
			}
			else if(rc == 0) return 0;
		}
		else
		{
			if(this->num_members != that->num_members)
			{
				LOG_WARNING("two object build from the same class %s has different memory structure", cesk_object_classpath(first));
				LOG_WARNING("first class has %d fields for class %s, but the second one has %d", 
							this->num_members, 
							cesk_object_classpath(first),
							that->num_members);
				return 0;
			}
			for(j = 0; j < this->num_members; j ++)
			{
				uint32_t addr1 = this->addrtab[j];
				uint32_t addr2 = that->addrtab[j];
				if(addr1 != addr2) return 0;
			}
			CESK_OBJECT_STRUCT_ADVANCE(this);
			CESK_OBJECT_STRUCT_ADVANCE(that);
		}
	}
	return 1;
}
const char* cesk_object_to_string(const cesk_object_t* object, char* buf, size_t sz, int brief)
{
	static char _buf[1024];
	if(NULL == buf)
	{
		buf = _buf;
		sz = sizeof(_buf);
	}
	char* p = buf;
	const cesk_object_struct_t* this = object->members;
	int i;
#define __PR(fmt, args...) do{\
	int pret = snprintf(p, buf + sz - p, fmt, ##args);\
	if(pret > buf + sz - p) pret = buf + sz - p;\
	p += pret;\
}while(0)
	if(brief)
	{
		__PR("[class %s]", this->class.path->value);
		return buf;
	}
	for(i = 0; i < object->depth; i ++)
	{
		__PR("[class %s (", this->class.path->value);
		if(this->built_in)
		{
			__PR("%s)]", bci_class_to_string(this->bcidata, NULL, 0, this->class.bci->class));
		}
		else
		{
			int j;
			for(j = 0; j < this->num_members; j ++)
			{
				__PR("(%s "PRSAddr") ", this->class.udef->members[j] ,this->addrtab[j]);
			}
			__PR(")]");
		}
		CESK_OBJECT_STRUCT_ADVANCE(this);
	}
	__PR("(tags %s)", tag_set_to_string(object->tags, NULL, 0));
#undef __PR
	return buf;
}
uint32_t cesk_object_instance_of(const cesk_object_t* object, const dalvik_type_t* type)
{
	const char* classpath = NULL;
	if(DALVIK_TYPECODE_ARRAY == type->typecode)
		classpath = NULL;
	else if(DALVIK_TYPECODE_OBJECT == type->typecode)
		classpath = type->data.object.path;
	else
	{
		LOG_ERROR("invalid operand");
		return CESK_STORE_ADDR_NULL;
	}
	const cesk_object_struct_t* this = object->members;
	int i;
	for(i = 0; i < object->depth; i ++)
	{
		int j;
		if(this->class.path->value == classpath) return 1;
		if(this->built_in)
		{
			int rc = bci_class_is_instance(this->bcidata, type, this->class.bci->class);
			if(rc > 0) 
			{
				switch(rc)
				{
					case BCI_BOOLEAN_TRUE: return CESK_STORE_ADDR_POS;
					case BCI_BOOLEAN_UNKNOWN: return CESK_STORE_ADDR_POS | CESK_STORE_ADDR_ZERO;
				}
			}
			else if(rc < 0)
			{
				LOG_ERROR("can not call function bci_class_instance_of for built-in class %s", classpath);
				return CESK_STORE_ADDR_NULL;
			}
		}
		else
		{
			for(j = 0; this->class.udef->implements[j] != NULL; j ++)
			{
				if(classpath == this->class.udef->implements[i]) return CESK_STORE_ADDR_POS;
			}
		}
		CESK_OBJECT_STRUCT_ADVANCE(this);
	}
	return CESK_STORE_ADDR_ZERO;
}
