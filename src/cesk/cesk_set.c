#include <log.h>
#include <cesk/cesk_set.h>
/* We do not maintain a hash table for each set, because
 * most set contains a few element. 
 * Instead of maintaining a hash table for each set,
 * we maintain a large hash table use <set_idx, address>
 * as key. 
 */
struct _cesk_set_t {
    uint32_t set_idx;    /* the set index */
};
typedef struct _cesk_set_node_t cesk_set_node_t;
/* data that the some set holds */
typedef struct {
    uint32_t set_idx;  /* set index */
    uint32_t addr;     /* the address this data entry refer */
} cesk_set_data_entry_t;;
typedef struct {
    uint32_t set_idx;       /* the set index */
    uint32_t size;          /* how many element in the set */
    uint32_t refcnt;        /* the reference count of this, indicates how many cesk_set_t for this set are returned */
    cesk_set_node_t* first; /* first element of the set */
} cesk_set_info_entry_t;
struct _cesk_set_node_t {
    /* these two pointers are used to maintain the member list */
    cesk_set_node_t *prev;
    cesk_set_node_t *next;
    /* this pointer is for the hash table */
    cesk_set_node_t *hash_next;
    /* the following space is for the actuall data */
    cesk_set_data_entry_t data_entry[0];   /* this is valid for a data entry node */
    cesk_set_info_entry_t info_entry[0];   /* this is valid for an info entry node */
};

static cesk_set_node_t* _cesk_set_hash[CESK_SET_HASH_SIZE];

#define DATA_ENTRY 0
#define INFO_ENTRY 1

static inline cesk_set_node_t* _cesk_set_node_alloc(int type)
{

}
/* allocate a fresh set index */
static inline uint32_t _cesk_set_idx_alloc()
{
    
}

