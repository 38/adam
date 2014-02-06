#include <cesk/cesk_store.h>
cesk_store_t* cesk_store_empty_store()
{
   cesk_store_t* ret = (cesk_store_t*)malloc(cesk_store_t*);
   if(NULL == ret)
   {
       LOG_ERROR("can not allocate memory for empty store");
       return NULL;
   }
   
}
