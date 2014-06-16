#include <string.h>
#include <stdlib.h>

#include <bci/bci_nametab.h>
/**
 * @brief the node in BCI name table
 **/
typedef struct _bci_nametab_node_t _bci_nametab_node_t;
struct _bci_nametab_node_t{
	const char* clspath;    /*!< the class path */
	const char* methname;   /*!< the method name, NULL if this is a class */
	void* def;              /*!< the definition */
	_bci_nametab_node_t* next; /*!< the next node in table */
};

static _bci_nametab_node_t* _bci_nametab[BCI_NAMETAB_SIZE];

int bci_nametab_init()
{
	memset(_bci_nametab, 0, sizeof(_bci_nametab));
	return 0;
}
void bci_nametab_finialize()
{
	_bci_nametab_node_t* ptr;
	int i;
	for(i = 0; i < BCI_NAMETAB_SIZE; i ++)
	{
		for(ptr = _bci_nametab[i]; NULL != ptr; )
		{
			_bci_nametab_node_t* this = ptr;
			ptr = ptr->next;
			if(NULL == this->methname)
			{
				bci_class_t* class = (bci_class_t*)ptr->def;
				if(class->ondelete) class->ondelete();
			}
			free(ptr->def);
			free(ptr);
		}
	}
}

/**
 * @brief the hash code for the BCI object
 * @param class_path the class path
 * @param method_name the method name
 * @return the hash code for this object
 **/
static inline hashval_t _bci_nametab_hash(const char* class_path, const char* method_name)
{
	hashval_t a = ((uintptr_t)class_path) & 0xffffffff;
	hashval_t b = ((uintptr_t)method_name) & 0xffffffff;
	return (a * a * 100003 * MH_MULTIPLY + b * MH_MULTIPLY );
}

/**
 * @brief allocate a new name table node
 * @param class the class paht
 * @param method the method name
 * @param object the object pointer
 * @return the newly created node, NULL indicates an error
 **/
static inline _bci_nametab_node_t* _bci_nametab_node_alloc(const char* class, const char* method, void* object)
{
	_bci_nametab_node_t* ret = (_bci_nametab_node_t*)malloc(sizeof(_bci_nametab_node_t));
	if(NULL == ret)
	{
		LOG_ERROR("can not allocate memory for the BCI name table node");
		return NULL;
	}
	ret->clspath = class;
	ret->methname = method;
	ret->def = object;
	ret->next = NULL;
	return ret;
}
/**
 * @brief insert a new BCI object to the table
 * @param class the class name
 * @param method the method name
 * @param object the BCI object
 * @return < 0 indicates error
 **/
static inline int _bci_nametab_insert(const char* class, const char* method, void* object)
{
	uint32_t h = _bci_nametab_hash(class, method) % BCI_NAMETAB_SIZE;
	_bci_nametab_node_t* node = _bci_nametab_node_alloc(class, method, object);
	if(NULL == node) return -1;
	node->next = _bci_nametab[h];
	_bci_nametab[h] = node;
	return 0;
}

/**
 * @brief find a node in the name table
 * @param class the class path
 * @param method the method name
 * @return the node found in the table, NULL indicates not found
 **/
static inline _bci_nametab_node_t* _bci_nametab_find(const char* class, const char* method)
{
	uint32_t h = _bci_nametab_hash(class, method) % BCI_NAMETAB_SIZE;
	_bci_nametab_node_t* ptr;
	for(ptr = _bci_nametab[h]; NULL != ptr; ptr = ptr->next)
		if(ptr->clspath == class && ptr->methname == method)
			return ptr;
	return NULL;
}

int bci_nametab_register_class(const char* clspath, bci_class_t* def)
{
	return _bci_nametab_insert(clspath, NULL, def);
}

int bci_nametab_register_method(const char* clspath, const char* mthname, bci_method_t* def)
{
	return _bci_nametab_insert(clspath, mthname, def);
}

const bci_class_t* bci_nametab_get_class(const char* clspath)
{
	_bci_nametab_node_t* ret = _bci_nametab_find(clspath, NULL);
	if(NULL == ret) return NULL;
	return (const bci_class_t*)ret->def;
}

const bci_method_t* bci_nametab_get_method(const char* clspath, const char* methname)
{
	_bci_nametab_node_t* ret = _bci_nametab_find(clspath, methname);
	if(NULL == ret) return NULL;
	return (const bci_method_t*)ret->def;
}
