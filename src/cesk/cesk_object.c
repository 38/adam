#include <cesk/cesk_object.h>
#include <dalvik/dalvik_class.h>
#include <dalvik/dalvik_method.h>
#include <dalvik/dalvik_field.h>
#include <dalvik/dalvik_memberdict.h>
#include <log.h>
cesk_object_t* cesk_object_new(const char* classpath)
{
    dalvik_class_t* target_class;  /* because class path is unique, so we are just excepting one result */
    if(NULL == dalvik_memberdict_get_class(classpath, &target_class, 1))
    {
        /* TODO: handle the built-in classes */
        LOG_ERROR("we can not find class %s", classpath);
        return NULL;
    }
    cesk_object_t* object = NULL;
    //object = (cesk_object_t*) malloc(
}
