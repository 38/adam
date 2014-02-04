#include <cesk/cesk_object.h>
#include <dalvik/dalvik_class.h>
#include <dalvik/dalvik_method.h>
#include <dalvik/dalvik_field.h>
#include <dalvik/dalvik_memberdict.h>
#include <log.h>
cesk_object_t* cesk_object_new(const char* classpath)
{
    int field_count = 0;
    int class_count = 0;
    dalvik_class_t classes[1024]; 
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
        classpath = target_class->super;
    }
    size_t size = sizeof(cesk_object_t) +                   /* header */
                  sizeof(cesk_object_struct_t) * class_count +     /* class header */
                  sizeof(cesk_value_set_t*) * field_count;   /* fields */

    cesk_object_t* object = (cesk_object_t*)malloc(sizeof(cesk_object_t));
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
            if(NULL == dalvik_memberdict_get_fields(classes[i]->path, classes[i]->member[j], &field, 1))
            {
                LOG_ERROR("We can not find field %s/%s", clases[i].path, classes[i].member[j]);
                continue;
            }
            base->valuelist[field->offset] = NULL;   /* TODO: create new value object */
            LOG_INFO("Create new filed %s/%s for class %s at offset %d", classes[0]->path, classes[i]->path, filed->offset);
        }
        base = (cesk_object_struct_t*)(base->valuelist + j);
    }
    //object = (cesk_object_t*) malloc(
}
