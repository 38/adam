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
typedef struct _dalvik_memberdict_node_list_t {
    void*       object;
    struct _dalvik_memberdict_node_list_t* next;
} dalvik_memberdict_node_list_t;
typedef struct _dalvik_memberdict_node_t {
    const char* class_path;
    const char* member_name;
    int         type;
    //void*       object;
    dalvik_memberdict_node_list_t*   list;
    struct _dalvik_memberdict_node_t* next;
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
            dalvik_memberdict_node_list_t* lptr;
            for(lptr = old->list; lptr;)
            {
                dalvik_memberdict_node_list_t* lold;
                lold = lptr;
                lptr = lptr->next;
                if(old->type == _TYPE_METHOD)
                    dalvik_method_free((dalvik_method_t*)lold->object);
                else if(old->type == _TYPE_FIELD)
                    dalvik_field_free((dalvik_field_t*)lold->object);
                else free(lold->object);
                free(lold);
            }
            free(old);
        }
    }
}

static inline uint32_t _dalvik_memberdict_hash(const char* class_path, const char* member_name, int type)
{
    /* it's fine when running on a 32 bit machine */
    static const uint32_t typeid[] = {0xe3defu, 0x12fcdeu, 0x2323feu};
    uint64_t a = (uint64_t)class_path;
    uint64_t b = (uint64_t)member_name;
    return ((a * 100003) + b)^typeid[type];
}

static inline int _dalvik_memberdict_register_object(const char* class_path, const char* object_name, int type, void* obj)
{
    int idx = _dalvik_memberdict_hash(class_path, object_name, type)%DALVIK_MEMBERDICT_SIZE;
    dalvik_memberdict_node_t* ptr;
    dalvik_memberdict_node_list_t* payload = (dalvik_memberdict_node_list_t*)malloc(sizeof(dalvik_memberdict_node_list_t));
    if(NULL == payload) return -1;
    payload->next = NULL;
    payload->object = obj;
    for(ptr = _dalvik_memberdict_hash_table[idx];
        NULL != ptr;
        ptr = ptr->next)
    {
        if(ptr->class_path == class_path && ptr->member_name == object_name && ptr->type == type)
        {
            /* Insert it to a existing list */
            payload->next = ptr->list;
            ptr->list = payload;
            return 0;
        }
    }
    ptr = (dalvik_memberdict_node_t*)malloc(sizeof(dalvik_memberdict_node_t));
    if(NULL == ptr)
    {
        free(payload);
        return -1;
    }
    ptr->class_path = class_path;
    ptr->member_name = object_name;
    ptr->type = type;
    ptr->list = payload;
    ptr->next = _dalvik_memberdict_hash_table[idx];
    _dalvik_memberdict_hash_table[idx] = ptr;
    return 0;
}

int dalvik_memberdict_register_method(
        const char* class_path, 
        dalvik_method_t* method)
{
    if(NULL == method) return -1;
    return _dalvik_memberdict_register_object(class_path, method->name, _TYPE_METHOD, method);
}


int dalvik_memberdict_register_field(
        const char*     class_path,
        dalvik_field_t* field)
{
    if(NULL == field) return -1;
    return _dalvik_memberdict_register_object(class_path, field->name, _TYPE_FIELD, field);
}

int dalvik_memberdict_register_class(
        const char*     class_path,
        dalvik_class_t* class)
{
    if(NULL == class) return -1;
    return _dalvik_memberdict_register_object(class_path, NULL, _TYPE_CLASS, class);
}

static inline void* _dalvik_memberdict_find_object(const char* path, const char* name, int type, void** buf, size_t bufsize)
{
    int idx = _dalvik_memberdict_hash(path, name, type)%DALVIK_MEMBERDICT_SIZE;
    dalvik_memberdict_node_t* ptr;
    for(ptr = _dalvik_memberdict_hash_table[idx];
        NULL != ptr;
        ptr = ptr->next)
    {
        if(ptr->class_path == path && ptr->member_name == name && ptr->type == type)  
        {
            //return ptr->object;
            dalvik_memberdict_node_list_t* lptr;
            for(lptr = ptr->list; lptr != NULL && bufsize > 1; bufsize --, buf ++)
                (*buf) = lptr->object;
        }
    }
    return NULL;
}


dalvik_method_t** dalvik_memberdict_get_methods(
        const char*     class_path,
        const char*     name,
        dalvik_method_t** buf,
        size_t bufsize)
{
    return (dalvik_method_t**)_dalvik_memberdict_find_object(class_path, name, _TYPE_METHOD, (void**)buf, bufsize);
}

dalvik_field_t** dalvik_memberdict_get_fields(
        const char*     class_path,
        const char*     name,
        dalvik_field_t** buf,
        size_t bufsize)
{
    return (dalvik_field_t**)_dalvik_memberdict_find_object(class_path, name, _TYPE_FIELD, (void**)buf, bufsize);
}


dalvik_class_t** dalvik_memberdict_get_classes(
        const char*     class_path,
        dalvik_class_t** buf,
        size_t bufsize)
{
    return (dalvik_class_t**)_dalvik_memberdict_find_object(class_path, NULL, _TYPE_CLASS, (void**)buf, bufsize);
}
