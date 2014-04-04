#include <dalvik/dalvik_exception.h>
#include <dalvik/dalvik_tokens.h>
#include <dalvik/dalvik_label.h>
#include <sexp.h>
#include <constants.h>
#include <vector.h>
#include <log.h>
#include <debug.h>


vector_t *_dalvik_exception_handler_vector     = NULL;
vector_t *_dalvik_exception_handler_set_vector = NULL;
int dalvik_exception_init()
{
	_dalvik_exception_handler_vector = vector_new(sizeof(dalvik_exception_handler_t*));
	_dalvik_exception_handler_set_vector = vector_new(sizeof(dalvik_exception_handler_set_t*));
	if(NULL == _dalvik_exception_handler_set_vector || NULL == _dalvik_exception_handler_set_vector)
	{
		LOG_ERROR("can not create new vector for exception handler");
		return -1;
	}
	return 0;
}
void dalvik_exception_finalize()
{
	if(NULL != _dalvik_exception_handler_vector)
	{
		size_t num_handler = vector_size(_dalvik_exception_handler_vector);
		int i;
		for(i = 0; i < num_handler; i ++)
		{
			dalvik_exception_handler_t* this = *(dalvik_exception_handler_t**)vector_get(_dalvik_exception_handler_vector, i);
			if(NULL != this) 
				free(this);
		}
		vector_free(_dalvik_exception_handler_vector);
	}
	if(NULL != _dalvik_exception_handler_set_vector)
	{
		size_t num_handlerset = vector_size(_dalvik_exception_handler_set_vector);
		int i;
		for(i = 0; i < num_handlerset; i ++)
		{
			dalvik_exception_handler_set_t* this = *(dalvik_exception_handler_set_t**)vector_get(_dalvik_exception_handler_set_vector, i);
			dalvik_exception_handler_set_t* ptr;
			for(ptr = this; NULL != ptr;)
			{
				dalvik_exception_handler_set_t* this = ptr;
				ptr = ptr->next;
				free(this);
			}
		}
		vector_free(_dalvik_exception_handler_set_vector);
	}
}
/** @brief allocate an exception handler */
static inline dalvik_exception_handler_t* _dalvik_exception_handler_alloc(const char* exception, int handler)
{
	dalvik_exception_handler_t* ret;
	ret = (dalvik_exception_handler_t*)malloc(sizeof(dalvik_exception_handler_t));
	if(NULL == ret) 
	{
		LOG_ERROR("can not allocate memory for exception");
		return NULL;
	}
	ret->exception = exception;
	ret->handler_label = handler;
	vector_pushback(_dalvik_exception_handler_vector, &ret);
	return ret;
}
/** @brief allocate an exception handler set */
static inline dalvik_exception_handler_set_t* _dalvik_exception_handler_set_alloc()
{
	dalvik_exception_handler_set_t* ret;
	ret = (dalvik_exception_handler_set_t*)malloc(sizeof(dalvik_exception_handler_set_t));
	ret->next = NULL;
	return ret;   /* Because we do not push the new allocated object into the vecotr,
					 So we should do it by callee */
}
dalvik_exception_handler_t* dalvik_exception_handler_from_sexp(const sexpression_t* sexp, int* from, int* to)   /* 2 Cases */
{
	if(SEXP_NIL == sexp ||
	   NULL     == from ||
	   NULL     == to) 
	{
		LOG_ERROR("invalid argument");
		return NULL;
	}
	if(sexp_match(sexp,"(L=A", DALVIK_TOKEN_CATCH, &sexp))
	{
		const char *label1, *label2, *label3;
		/* (catch classpath from label1 to label2 using label3) */
		const char* classpath = sexp_get_object_path(sexp, &sexp);
		if(NULL == classpath) return NULL;
		if(!sexp_match(sexp, "(L=L?L=L?L=L?", 
					DALVIK_TOKEN_FROM,
					&label1,
					DALVIK_TOKEN_TO,
					&label2,
					DALVIK_TOKEN_USING,
					&label3)) 
		{
			LOG_ERROR("can not parse exception handler");
			LOG_DEBUG("the exception handler is %s", sexp_to_string(sexp,NULL));
			return NULL;
		}
		int lid1, lid2, lid3;

		lid1 = dalvik_label_get_label_id(label1);
		if(lid1 < 0) 
		{
			LOG_ERROR("invalid label");
			LOG_DEBUG("the exception handler is %s", sexp_to_string(sexp,NULL));
			return NULL;
		}
		lid2 = dalvik_label_get_label_id(label2);
		if(lid2 < 0) 
		{
			LOG_ERROR("invalid label");
			LOG_DEBUG("the exception handler is %s", sexp_to_string(sexp,NULL));
			return NULL;
		}
		lid3 = dalvik_label_get_label_id(label2);
		if(lid3 < 0) 
		{
			LOG_ERROR("invalid label");
			LOG_DEBUG("the exception handler is %s", sexp_to_string(sexp,NULL));
			return NULL;
		}
		(*from) = lid1;
		(*to) = lid2;
		dalvik_exception_handler_t* handler = _dalvik_exception_handler_alloc(classpath, lid3);
		return handler;
	}
	else if(sexp_match(sexp, "(L=A", DALVIK_TOKEN_CATCHALL, &sexp))
	{
		const char *label1, *label2, *label3;
		/* (catch classpath from label1 to label2 using label3) */
		const char* classpath = sexp_get_object_path(sexp, &sexp);
		if(NULL == classpath) return NULL;
		if(!sexp_match(sexp,"(L=L?L=L?L=L?", 
					DALVIK_TOKEN_FROM,
					&label1,
					DALVIK_TOKEN_TO,
					&label2,
					DALVIK_TOKEN_USING,
					&label3)) 
		{
			LOG_ERROR("can not parse exception handler");
			LOG_DEBUG("the exception handler is %s", sexp_to_string(sexp,NULL));
			return NULL;
		}
		int lid1, lid2, lid3;

		lid1 = dalvik_label_get_label_id(label1);
		if(lid1 < 0) 
		{
			return NULL;
		}
		lid2 = dalvik_label_get_label_id(label2);
		if(lid2 < 0) 
		{
			return NULL;
		}
		lid3 = dalvik_label_get_label_id(label2);
		if(lid3 < 0) 
		{
			LOG_ERROR("invalid label");
			LOG_DEBUG("the exception handler is %s", sexp_to_string(sexp,NULL));
			return NULL;
		}
		(*from) = lid1;
		(*to) = lid2;
		dalvik_exception_handler_t* handler = _dalvik_exception_handler_alloc(NULL, lid3);
		return handler;
	}
	LOG_ERROR("unreachable code");
	return NULL;
}

dalvik_exception_handler_set_t* dalvik_exception_new_handler_set(size_t count, dalvik_exception_handler_t** set)
{
	if(count == 0) return NULL;
	int i;
	dalvik_exception_handler_set_t* ret = NULL;
	if(count < 0) 
	{
		LOG_ERROR("number of handlers can not be negative");
		goto ERR;
	}
	for(i = count - 1; i >= 0; i --)
	{
		dalvik_exception_handler_set_t* this = _dalvik_exception_handler_set_alloc();
		if(NULL == this) 
		{
			LOG_ERROR("can not allocate memory");
			goto ERR;
		}
		this->next = ret;
		ret = this;
	}
	vector_pushback(_dalvik_exception_handler_set_vector, &ret);
	return ret;
ERR:
	for(; ret != NULL;)
	{
		dalvik_exception_handler_set_t* this = ret;
		ret = ret->next;
		free(this);
	}
	return NULL;
}
