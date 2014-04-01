#include <string.h>

#include <log.h>

#include <dalvik/dalvik_class.h>
#include <dalvik/dalvik_method.h>
#include <dalvik/dalvik_field.h>
#include <dalvik/dalvik_memberdict.h>

#include <cesk/cesk_set.h>
#include <cesk/cesk_object.h>
cesk_object_t* cesk_object_new(const char* classpath)
{
	int field_count = 0;
	int class_count = 0;
	dalvik_class_t* classes[1024]; 
	/* find classes inhernt relationship, and determin the memory layout */
	for(;;)
	{
		LOG_NOTICE("try to find class %s", classpath);
		dalvik_class_t* target_class = dalvik_memberdict_get_class(classpath);
		if(NULL == target_class)
		{
			/* TODO: handle the built-in classes */
			LOG_WARNING("can not find class %s", classpath);
			LOG_NOTICE("fixme: here we should handle the built-in classes");
			break;
		}
		int i;
		for(i = 0; target_class->members[i]; i ++)
			field_count ++;
		classes[class_count ++] = target_class;
		if(class_count >= 1024)
		{
			LOG_ERROR("a class with inherent from more than 1024 classes? are you kidding me?");
			return NULL;
		}
		LOG_INFO("found class %s at %p", classpath, target_class);
		classpath = target_class->super;
	}
	/* compute the size required for this instance */
	size_t size = sizeof(cesk_object_t) +                   	   /* header */
				  sizeof(cesk_object_struct_t) * class_count +     /* class header */
				  sizeof(cesk_set_t*) * field_count;   			   /* fields */
	/* okay, create the new object */
	cesk_object_t* object = (cesk_object_t*)malloc(size);
	cesk_object_struct_t* base = object->members; 
	if(NULL == object)
	{
		LOG_ERROR("can not allocate memory for new object %s", classes[0]->path);
		return NULL;
	}
	int i;
	for(i = 0; i < class_count; i ++)
	{
		int j;
		for(j = 0; classes[i]->members[j]; j ++)
		{
			dalvik_field_t* field = dalvik_memberdict_get_field(classes[i]->path, classes[i]->members[j]);   /* because only function can overload */
			if(NULL == field)
			{
				LOG_WARNING("Can not find field %s/%s, skip", classes[i]->path, classes[i]->members[j]);
				continue;
			}
			base->valuelist[field->offset] = CESK_STORE_ADDR_NULL;  
			LOG_DEBUG("Create new filed %s/%s for class %s at offset %d", classes[i]->path, 
																		 classes[i]->members[j], 
																		 classes[0]->path, 
																		 field->offset);
		}
		base->class = classes[i];
		base->num_members = j;
		CESK_OBJECT_STRUCT_ADVANCE(base);
	}
	object->depth = class_count;
	object->size = size;
	return object;
}
uint32_t* cesk_object_get(cesk_object_t* object, const char* classpath, const char* field_name)
{
	if(NULL == object    ||
	   NULL == classpath ||
	   NULL == field_name)
	{
		LOG_ERROR("invalid arguments");
		return NULL;
	}
	int i;
	cesk_object_struct_t* this = object->members;
	for(i = 0; i < object->depth; i ++)
	{
		if(this->class->path == classpath) 
		{
			break;
		}
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
	
	dalvik_field_t* field = dalvik_memberdict_get_field(classpath, field_name);
	if(NULL == field)
	{
		LOG_WARNING("No field named %s/%s", classpath, field_name);
		return NULL;
	}
	return this->valuelist + field->offset;
}
void cesk_object_free(cesk_object_t* object)
{
	if(NULL == object) return;
	free(object);   /* the object occupies consequence memory */
}
cesk_object_t* cesk_object_fork(const cesk_object_t* object)
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
	return newobj;
}

hashval_t cesk_object_hashcode(const cesk_object_t* object)
{
	hashval_t  hash = ((uintptr_t)object->members[0].class->path) & ~(hashval_t)0;    /* We also consider the type of the object */
	
	int i;
	const cesk_object_struct_t* this = object->members;
	uint32_t mul = MH_MULTIPLY;
	for(i = 0; i < object->depth; i ++)
	{
		int j;
		mul ^= (uintptr_t)this->class->path;
		for(j = 0; j < this->num_members; j ++)
		{
			hashval_t k = this->valuelist[j] * mul;
			mul *= MH_MULTIPLY;
			hash ^= k;
			hash = (hash << 16) | (hash >> 16);
		}
		CESK_OBJECT_STRUCT_ADVANCE(this);
	}
	return hash;
}
hashval_t cesk_object_compute_hashcode(const cesk_object_t* object)
{
	/* the object hash itself is non-incremental style, so call
	 * the hashcode function directly */
	return cesk_object_hashcode(object);
}
int cesk_object_equal(const cesk_object_t* first, const cesk_object_t* second)
{
	if(NULL == first || NULL == second) return first == second;
	if(first->members[0].class->path != second->members[0].class->path) return 0;
	const cesk_object_struct_t* this = first->members;
	const cesk_object_struct_t* that = second->members;

	/* if the class path is the same, we assume that 
	 * two object has the same depth  */
	int i;
	if(first->depth != second->depth)
	{
		LOG_WARNING("two object build from the same class %s has defferent inherent depth", cesk_object_classpath(first));
		return 0;
	}
	/* do actuall comparasion */
	for(i = 0; i < first->depth; i ++)
	{
		int j;
		if(this->num_members != that->num_members)
		{
			LOG_WARNING("two object build from the same class %s has different memory structure", cesk_object_classpath(first));
			LOG_WARNING("first class has %zu fields for class %s, but the second one has %zu", 
						this->num_members, 
						cesk_object_classpath(first),
						that->num_members);
			return 0;
		}
		for(j = 0; j < this->num_members; j ++)
		{
			uint32_t addr1 = this->valuelist[j];
			uint32_t addr2 = that->valuelist[j];
			if(addr1 != addr2) return 0;
		}
		CESK_OBJECT_STRUCT_ADVANCE(this);
		CESK_OBJECT_STRUCT_ADVANCE(that);
	}
	return 1;
}
const char* cesk_object_to_string(const cesk_object_t* object, char* buf, size_t sz)
{
	static char _buf[1024];
	if(NULL == buf)
	{
		buf = _buf;
		sz = sizeof(_buf);
	}
	char* p = _buf;
#define __PR(fmt, args...) do{p += snprintf(p, buf + sz - p, fmt, ##args);}while(0)
	const cesk_object_struct_t* this = object->members;
	int i;
	for(i = 0; i < object->depth; i ++)
	{
		__PR("[class %s (", this->class->path);
		int j;
		for(j = 0; j < this->num_members; j ++)
		{
			__PR("%d ", this->valuelist[j]);
		}
		__PR(")]");
		CESK_OBJECT_STRUCT_ADVANCE(this);
	}
#undef __PR
	return buf;
}
int cesk_object_instance_of(const cesk_object_t* object, const char* classpath)
{
	const cesk_object_struct_t* this = object->members;
	int i;
	for(i = 0; i < object->depth; i ++)
	{
		int j;
		if(this->class->path == classpath) return 1;
		for(j = 0; this->class->implements[j] != NULL; j ++)
		{
			if(classpath == this->class->implements[i]) return 1;
		}
	}
	return 0;
}
