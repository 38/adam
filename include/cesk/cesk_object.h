#ifndef __CESK_OBJECT_H__
#define __CESK_OBJECT_H__
#include <constants.h>

#include <cesk/cesk_value.h>

/* a abstract object value */
typedef struct {
    const char*      classpath;   /* the class path of this object */
    size_t           num_members; /* the number of members */
    cesk_value_set_t members[0];  /* the value of the member */
} cesk_object_t;  

cesk_object_t* cesk_object_new(const char* classpath);
#endif
