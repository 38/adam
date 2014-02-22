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
    for(;;)
    {
        LOG_NOTICE("try to find class %s", classpath);
        dalvik_class_t* target_class = dalvik_memberdict_get_class(classpath);
        if(NULL == target_class)
        {
            /* TODO: handle the built-in classes */
            LOG_WARNING("we can not find class %s", classpath);
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
    size_t size = sizeof(cesk_object_t) +                   /* header */
                  sizeof(cesk_object_struct_t) * class_count +     /* class header */
                  sizeof(cesk_set_t*) * field_count;   /* fields */

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
            LOG_INFO("Create new filed %s/%s for class %s at offset %d", classes[i]->path, 
                                                                         classes[i]->members[j], 
                                                                         classes[0]->path, 
                                                                         field->offset);
        }
        base->class = classes[i];
        base->num_members = j;
        base = (cesk_object_struct_t*)(base->valuelist + j);
    }
    object->depth = class_count;
    return object;
}
uint32_t* cesk_object_get(cesk_object_t* object, const char* classpath, const char* field_name)
{
    if(NULL == object    ||
       NULL == classpath ||
       NULL == field_name) 
        return NULL;
    int i;
    cesk_object_struct_t* this = object->members;
    for(i = 0; i < object->depth; i ++)
    {
        if(this->class->path == classpath) 
        {
            break;
        }
        this = (cesk_object_struct_t*)(this->valuelist + this->num_members);
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
cesk_object_t* cesk_object_fork(cesk_object_t* object)
{
    cesk_object_struct_t* base = object->members;
    size_t objsize = sizeof(cesk_object_t);
    LOG_TRACE("copy object %s@%p", base->class->path, object);
    int i;
    for(i = 0; i < object->depth; i ++)
    {
        objsize += sizeof(cesk_object_struct_t) + sizeof(uint32_t) * base->num_members;
        base = (cesk_object_struct_t*)(base->valuelist + base->num_members);
    }
    cesk_object_t* newobj = (cesk_object_t*)malloc(objsize);
    if(NULL == newobj) return NULL;
    memcpy(newobj, object, objsize);
    return newobj;
}

hashval_t cesk_object_hashcode(cesk_object_t* object)
{
    hashval_t  hash = ((uintptr_t)object->members[0].class->path) & ~(hashval_t)0;    /* We also consider the type of the object */
    
    int i;
    cesk_object_struct_t* this = object->members;
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
        this = (cesk_object_struct_t*)(this->valuelist + this->num_members);
    }
    return hash;
}
int cesk_object_equal(cesk_object_t* first, cesk_object_t* second)
{
    if(NULL == first || NULL == second) return first == second;
    if(first->members[0].class->path != second->members[0].class->path) return 0;
    cesk_object_struct_t* this = first->members;
    cesk_object_struct_t* that = second->members;

    /* if the class path is the same, we assume that 
     * two object has the same depth 
     */
    int i;
    if(first->depth != second->depth)
    {
        LOG_WARNING("two object build from the same class %s has defferent inherent depth", cesk_object_classpath(first));
        return 0;
    }
    for(i = 0; i < first->depth; i ++)
    {
        int j;
        if(this->num_members != that->num_members)
        {
            LOG_WARNING("two object build from the same class %s has different memory structure", cesk_object_classpath(first));
            LOG_WARNING("first class has %d fields for class %d, but the second one has %d", 
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
        this = (cesk_object_struct_t*)(this->valuelist + this->num_members);
        that = (cesk_object_struct_t*)(that->valuelist + that->num_members);
    }
    return 1;
}
const char* cesk_object_to_string(cesk_object_t* object, char* buf, size_t sz)
{
    static char _buf[1024];
    if(NULL == buf)
    {
        buf = _buf;
        sz = sizeof(_buf);
    }
    char* p = _buf;
#define __PR(fmt, args...) do{p += snprintf(p, buf + sz - p, fmt, ##args);}while(0)
    cesk_object_struct_t* this = object->members;
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
        this = (cesk_object_struct_t*)(this->valuelist + this->num_members);
    }
#undef __PR
    return buf;
}
int cesk_object_instance_of(cesk_object_t* object, const char* classpath)
{
	cesk_object_struct_t* this = object->members;
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
