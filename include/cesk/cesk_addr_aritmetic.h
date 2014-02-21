#ifndef __CESK_ADDR_ARITHMETIC_H__
#define __CESK_ADDR_ARITHMETIC_H__
#include <cesk/cesk_store.h>
/* constant address arithmetic */
/* returns address for -a */
static inline uint32_t cesk_addr_arithmetic_neg(uint32_t a)
{
	uint32_t ret = CESK_STORE_ADDR_CONST_PREFIX;
	if(CESK_STORE_ADDR_CONST_CONTAIN(a, NEG))
		ret = CESK_STORE_ADDR_CONST_SET(ret, POS);
	if(CESK_STORE_ADDR_CONST_CONTAIN(a, POS));
		ret = CESK_STORE_ADDR_CONST_SET(ret, NEG);
	if(CESK_STORE_ADDR_CONST_CONTAIN(ret, ZERO))
		ret = CESK_STORE_ADDR_CONST_SET(ret, ZERO);
	if(ret == CESK_STORE_ADDR_CONST_PREFIX)
	{
		LOG_WARNING("an empty constant returned, the input address might be wrong ( which is %x)", a);
	}
	return ret;
}
static inline uint32_t cesk_addr_arithmetic_add(uint32_t a, uint32_t b)
{
	int ret = CESK_STORE_ADDR_CONST_PREFIX;
	/* neg + neg = neg */
	if(CESK_STORE_ADDR_CONST_CONTAIN(a, NEG) &&
	   CESK_STORE_ADDR_CONST_CONTAIN(b, NEG))
		ret = CESK_STORE_ADDR_CONST_SET(ret, NEG);
	/* neg + zero = neg */
	if(CESK_STORE_ADDR_CONST_CONTAIN(a, NEG) &&
	   CESK_STORE_ADDR_CONST_CONTAIN(b, ZERO))
		ret = CESK_STORE_ADDR_CONST_SET(ret, NEG);
	/* neg + pos = {neg, pos, zero} */
	if(CESK_STORE_ADDR_CONST_CONTAIN(a, NEG) &&
	   CESK_STORE_ADDR_CONST_CONTAIN(b, POS))
	{
		ret = CESK_STORE_ADDR_CONST_SET(ret, NEG);
		ret = CESK_STORE_ADDR_CONST_SET(ret, POS);
		ret = CESK_STORE_ADDR_CONST_SET(ret, ZERO);
	}
	/* zero + neg = neg */
	if(CESK_STORE_ADDR_CONST_CONTAIN(a, ZERO) &&
	   CESK_STORE_ADDR_CONST_CONTAIN(b, NEG))
		ret = CESK_STORE_ADDR_CONST_SET(ret, NEG);
	/* zero + zero = zero */
	if(CESK_STORE_ADDR_CONST_CONTAIN(a, ZERO) &&
	   CESK_STORE_ADDR_CONST_CONTAIN(b, ZERO))
		ret = CESK_STORE_ADDR_CONST_SET(ret, ZERO);
	/* zero + pos = pos */
	if(CESK_STORE_ADDR_CONST_CONTAIN(a, ZERO) &&
	   CESK_STORE_ADDR_CONST_CONTAIN(b, POS))
		ret = CESK_STORE_ADDR_CONST_SET(ret, POS);
	/* pos + neg = {neg, pos, zero} */
	if(CESK_STORE_ADDR_CONST_CONTAIN(a, POS) &&
	   CESK_STORE_ADDR_CONST_CONTAIN(b, NEG))
	{
		ret = CESK_STORE_ADDR_CONST_SET(ret, NEG);
		ret = CESK_STORE_ADDR_CONST_SET(ret, POS);
		ret = CESK_STORE_ADDR_CONST_SET(ret, ZERO);
	}
	/* pos + zero = pos */
	if(CESK_STORE_ADDR_CONST_CONTAIN(a, POS) &&
	   CESK_STORE_ADDR_CONST_CONTAIN(b, ZERO))
		ret = CESK_STORE_ADDR_CONST_SET(ret, POS);
	/* pos + pos = pos */
	if(CESK_STORE_ADDR_CONST_CONTAIN(a, POS) &&
	   CESK_STORE_ADDR_CONST_CONTAIN(b, POS))
		ret = CESK_STORE_ADDR_CONST_SET(ret, POS);
	if(CESK_STORE_ADDR_CONST_PREFIX == ret)
	{
		LOG_WARNING("an empty constant set returnted, it might be a mistake");
		return CESK_STORE_ADDR_CONST_PREFIX;
	}
	return ret;
}
static inline uint32_t cesk_addr_arithmetic_sub(uint32_t a, uint32_t b)
{
	return cesk_addr_arithmetic_add(a, cesk_addr_arithmetic_neg(b));
}
static inline uint32_t cesk_addr_arithmetic_mul(uint32_t a, uint32_t b)
{
	int ret = CESK_STORE_ADDR_CONST_PREFIX;
	/* neg x neg = pos */
	if(CESK_STORE_ADDR_CONST_CONTAIN(a, NEG) &&
	   CESK_STORE_ADDR_CONST_CONTAIN(b, NEG))
		ret = CESK_STORE_ADDR_CONST_SET(ret, POS);
	/* neg x zero = zero */
	if(CESK_STORE_ADDR_CONST_CONTAIN(a, NEG) &&
	   CESK_STORE_ADDR_CONST_CONTAIN(b, ZERO))
		ret = CESK_STORE_ADDR_CONST_SET(ret, ZERO);
	/* neg x pos = neg */
	if(CESK_STORE_ADDR_CONST_CONTAIN(a, NEG) &&
	   CESK_STORE_ADDR_CONST_CONTAIN(b, POS))
		ret = CESK_STORE_ADDR_CONST_SET(ret, NEG);
	/* zero x neg = zero */
	if(CESK_STORE_ADDR_CONST_CONTAIN(a, ZERO) &&
	   CESK_STORE_ADDR_CONST_CONTAIN(b, NEG))
		ret = CESK_STORE_ADDR_CONST_SET(ret, ZERO);
	/* zero x zero = zero */
	if(CESK_STORE_ADDR_CONST_CONTAIN(a, ZERO) &&
	   CESK_STORE_ADDR_CONST_CONTAIN(b, ZERO))
		ret = CESK_STORE_ADDR_CONST_SET(ret, ZERO);
	/* zero x pos = zero */
	if(CESK_STORE_ADDR_CONST_CONTAIN(a, ZERO) &&
	   CESK_STORE_ADDR_CONST_CONTAIN(b, POS))
		ret = CESK_STORE_ADDR_CONST_SET(ret, ZERO);
	/* pos x neg =  neg */
	if(CESK_STORE_ADDR_CONST_CONTAIN(a, POS) &&
	   CESK_STORE_ADDR_CONST_CONTAIN(b, NEG))
		ret = CESK_STORE_ADDR_CONST_SET(ret, NEG);
	/* pos x zero = zero */
	if(CESK_STORE_ADDR_CONST_CONTAIN(a, POS) &&
	   CESK_STORE_ADDR_CONST_CONTAIN(b, ZERO))
		ret = CESK_STORE_ADDR_CONST_SET(ret, ZERO);
	/* pos x pos = pos */
	if(CESK_STORE_ADDR_CONST_CONTAIN(a, POS) &&
	   CESK_STORE_ADDR_CONST_CONTAIN(b, POS))
		ret = CESK_STORE_ADDR_CONST_SET(ret, POS);
	if(CESK_STORE_ADDR_CONST_PREFIX == ret)
	{
		LOG_WARNING("an empty constant set returnted, it might be a mistake");
		return CESK_STORE_ADDR_CONST_PREFIX;
	}
	return ret;
}
static inline cesk_addr_arithmetic_div(uint32_t a, uint32_t b)
{
	/* we do not want 0 in the set */
	b &= (CESK_STORE_ADDR_CONST_PREFIX | 
		 CESK_STORE_ADDR_CONST_SUFFIX(CESK_STORE_ADDR_NEG) |
		 CESK_STORE_ADDR_CONST_SUFFIX(CESK_STORE_ADDR_POS));
    if(CESK_STORE_ADDR_CONST_PREFIX == b)
	{
		LOG_WARNING("divided by zero");
	}
	return cesk_addr_arithmetic_mul(a,b);
}
static inline cesk_addr_arithmetic_not(uint32_t a, uint32_t b)
{
	ret = CESK_STORE_ADDR_CONST_PREFIX;
	if(CESK_STORE_ADDR_CONST_CONTAIN(a, TRUE)) 
		ret = CESK_STORE_ADDR_CONST_SET(ret, FALSE);
	if(CESK_STORE_ADDR_CONST_CONTAIN(a, FALSE)) 
		ret = CESK_STORE_ADDR_CONST_SET(ret, TRUE);
	return ret;
}
static inline cesk_addr_arithmetic_and(uint32_t a, uint32_t b)
{
	ret = CESK_STORE_ADDR_CONST_PREFIX;
	if(CESK_STORE_ADDR_CONST_CONTAIN(a, TRUE) &&
			//TODO
}
#endif
