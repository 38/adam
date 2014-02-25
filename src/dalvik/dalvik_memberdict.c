#include <dalvik/dalvik_memberdict.h>
#include <log.h>
#include <stringpool.h>
#include <string.h>
#include <dalvik/dalvik_class.h>
#include <dalvik/dalvik_method.h>
#include <dalvik/dalvik_field.h>
#include <debug.h>

#define _TYPE_METHOD 0
#define _TYPE_FIELD 1
#define _TYPE_CLASS 2
<<<<<<< HEAD

typedef struct _dalvik_memberdict_node_t {
    int type;
    const char* class_path;
    const char* member_name;
    void*       object;
    struct _dalvik_memberdict_node_t* next;
=======
typedef struct _dalvik_memberdict_node_t {
    const char* class_path;             /* the class path */
    const char* member_name;            /* the name of the member */
    const dalvik_type_t* const * args;        /* the type list (only valid for method, otherwise set to NULL */
    int         type;                   /* type of the object method, field or class */
    void*       object;                 /* the storage of the object */
    struct _dalvik_memberdict_node_t* next; 
>>>>>>> change member dict to support reload
} dalvik_memberdict_node_t;

dalvik_memberdict_node_t* _dalvik_memberdict_hash_table[DALVIK_MEMBERDICT_SIZE];



void dalvik_memberdict_init()
{
    memset(_dalvik_memberdict_hash_table, 0, sizeof(_dalvik_memberdict_hash_table));
}
void dalvik_memberdict_finalize()
{
    int i;
    for(i = 0; i < DALVIK_MEMBERDICT_SIZE; i ++)
    {
        dalvik_memberdict_node_t* ptr;
        for(ptr = _dalvik_memberdict_hash_table[i];
            NULL != ptr;)
        {
            dalvik_memberdict_node_t* old = ptr;
            ptr = ptr->next;
            switch(old->type)
            {
                case _TYPE_METHOD:
                    dalvik_method_free((dalvik_method_t*)old->object);
                    break;
                case _TYPE_FIELD:
                    dalvik_field_free((dalvik_field_t*)old->object);
                    break;
                case _TYPE_CLASS:
                    /* class type is just a simple list */
                    free(old->object); 
                    break;
                default:
                    LOG_ERROR("unknown node type in member dict, do not know how to free it");
            }
			free(old);
        }
    }
}

static inline hashval_t _dalvik_memberdict_hash(const char* class_path, const char* member_name, const dalvik_type_t* const * args, int type)
{
    /* it's fine when running on a 32 bit machine */
    static const uint32_t typeid[] = {0xe3deful, 0x12fcdeul, 0x2323feul};
    hashval_t a = ((uintptr_t)class_path) & 0xffffffff;
    hashval_t b = ((uintptr_t)member_name) & 0xffffffff;
    hashval_t c = dalvik_type_list_hashcode(args);
    return (a * a * 100003 + b * MH_MULTIPLY + c)^typeid[type];
}

static inline int _dalvik_memberdict_register_object(const char* class_path, const char* object_name, const dalvik_type_t * const * args ,int type, void* obj)
{
    uint32_t idx = _dalvik_memberdict_hash(class_path, object_name, args,type)%DALVIK_MEMBERDICT_SIZE;
    dalvik_memberdict_node_t* ptr;
    for(ptr = _dalvik_memberdict_hash_table[idx]; NULL != ptr; ptr = ptr->next)
    {
        if(ptr->class_path == class_path && 
           ptr->member_name == object_name && 
           dalvik_type_list_equal(args, ptr->args))
        {
            LOG_ERROR("can not register object %s.%s twice", class_path, object_name);
            return -1;
        }
    }
    ptr = (dalvik_memberdict_node_t*)malloc(sizeof(dalvik_memberdict_node_t));
    ptr->class_path = class_path;
    ptr->member_name = object_name;
    ptr->args = args;
    ptr->object = obj;
    ptr->type = type;
    ptr->next = _dalvik_memberdict_hash_table[idx];
    _dalvik_memberdict_hash_table[idx] = ptr;
    LOG_DEBUG("class member %s.%s is registered", ptr->class_path, ptr->member_name);
    return 0;
}

int dalvik_memberdict_register_method(const char* class_path, dalvik_method_t* method)
{
    if(NULL == method) return -1;
    return _dalvik_memberdict_register_object(class_path, method->name, method->args_type ,_TYPE_METHOD, method);
}


int dalvik_memberdict_register_field(const char* class_path, dalvik_field_t* field)
{
    if(NULL == field) return -1;
    return _dalvik_memberdict_register_object(class_path, field->name, NULL,_TYPE_FIELD, field);
}

int dalvik_memberdict_register_class(
        const char*     class_path,
        dalvik_class_t* class)
{
    if(NULL == class) return -1;
    return _dalvik_memberdict_register_object(class_path, NULL, NULL,_TYPE_CLASS, class);
}

static inline void* _dalvik_memberdict_find_object(const char* path, const char* name, const dalvik_type_t * const * args, int type)
{
    uint32_t idx = _dalvik_memberdict_hash(path, name, args ,type)%DALVIK_MEMBERDICT_SIZE;
    dalvik_memberdict_node_t* ptr;
    for(ptr = _dalvik_memberdict_hash_table[idx];
        NULL != ptr;
        ptr = ptr->next)
    {
        if(ptr->class_path == path && ptr->member_name == name && dalvik_type_list_equal(ptr->args, args))  
        {
            return ptr->object;
        }
    }
    return NULL;
}


dalvik_method_t* dalvik_memberdict_get_method(const char* class_path, const char* name, const dalvik_type_t * const * args)
{
    return (dalvik_method_t*)_dalvik_memberdict_find_object(class_path, name, args,_TYPE_METHOD);
}

dalvik_field_t* dalvik_memberdict_get_field(const char* class_path, const char* name)
{
    return (dalvik_field_t*)_dalvik_memberdict_find_object(class_path, name, NULL, _TYPE_FIELD);
}

dalvik_class_t* dalvik_memberdict_get_class(const char* class_path)
{
    return (dalvik_class_t*)_dalvik_memberdict_find_object(class_path, NULL, NULL, _TYPE_CLASS);
}
