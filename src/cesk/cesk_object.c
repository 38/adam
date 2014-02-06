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
        dalvik_class_t* target_class;  /* because class path is unique, so we are just excepting one result */
        if(NULL == dalvik_memberdict_get_classes(classpath, &target_class, 1))
        {
            /* TODO: handle the built-in classes */
            LOG_WARNING("we can not find class %s", classpath);
            break;
        }
        int i;
        for(i = 0; target_class->members[i]; i ++)
            field_count ++;
        classes[class_count ++] = target_class;
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
            dalvik_field_t* field = NULL;   /* because only function can overload */
            if(NULL == dalvik_memberdict_get_fields(classes[i]->path, classes[i]->members[j], &field, 1))
            {
                LOG_ERROR("We can not find field %s/%s", classes[i]->path, classes[i]->members[j]);
                continue;
            }
            base->valuelist[field->offset] = 0xfffffffful;  /* an invalid virtual address in frame store : TODO*/
            LOG_INFO("Create new filed %s/%s for class %s at offset %d", classes[i]->path, 
                                                                         classes[i]->members[j], 
                                                                         classes[0]->path, 
                                                                         field->offset);
        }
        base->classpath = classes[i]->path;
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
        if(this->classpath == classpath) 
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
                    object->members[0].classpath);
        return NULL;
    }
    
    dalvik_field_t* field;   /* because we don't allow define a same field twice */
    if(NULL == dalvik_memberdict_get_fields(classpath, field_name, &field, 1))
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
    LOG_TRACE("copy object %s@%p", base->classpath, object);
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
#if 0
/* use a Murmur Hash Function */
hashval_t cesk_object_hashcode(cesk_object_t* object)
{
    hashval_t  hash = ((uint64_t)object) & ~(hashval_t)0;    /* We also consider the type of the object */
    
    int i;
    int len = 0;

    cesk_object_struct_t* this = object->members;
    for(i = 0; i < object->depth; i ++)
    {
        int j;
        for(j = 0; j < this->num_members; j ++)
        {
            hashval_t k = /*TODO cesk_value_set_hashcode(this->valuelist[j])*/;
            k *= STRINGPOOL_MURMUR_C1;
            k = (k << STRINGPOOL_MURMUR_R1) | (k >> (32 - STRINGPOOL_MURMUR_R1));
            k *= STRINGPOOL_MURMUR_C2;
            hash ^= k;
            hash = (hash << STRINGPOOL_MURMUR_R2) | (k >> (32 - STRINGPOOL_MURMUR_R2));
            hash = hash * STRINGPOOL_MURMUR_M + STRINGPOOL_MURMUR_N;
            len ++;
        }
        this = (cesk_object_struct_t*)(this->valuelist + this->num_members);
    }
    hash ^= len;
    hash = hash ^ ( hash > 16);
    hash *= 0x85ebca6b;
    hash ^= (hash >> 13);
    hash *= 0xc2b2ae35;
    hash ^= (hash >> 16);
    return hash;
}
#endif
