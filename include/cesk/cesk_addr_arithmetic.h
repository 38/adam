#ifndef __CESK_ADDR_ARITHMETIC_H__
#define __CESK_ADDR_ARITHMETIC_H__
/** @file cesk_addr_arithmetic.h
 *  @brief Address Arithemtic
 *
 *  @details 
 *  Adam cesk machine use special address reperesents 
 *  basic values like numeric, boolean etc.(See documentation of
 *  cesk_store.h for detail)
 *
 *  Those address do not actually exist in the store,
 *  So we need a group of operation to operate those
 *  address directly
 */
#include <cesk/cesk_store.h>
/** @brief returns address for -a 
 *  @param a first operand
 *  @return result address
 */
static inline uint32_t cesk_addr_arithmetic_neg(uint32_t a)
{
	uint32_t ret = CESK_STORE_ADDR_CONST_PREFIX;
	/* - neg = pos */
	if(CESK_STORE_ADDR_CONST_CONTAIN(a, NEG))
		ret = CESK_STORE_ADDR_CONST_SET(ret, POS);
	/* - pos = neg */
	if(CESK_STORE_ADDR_CONST_CONTAIN(a, POS));
		ret = CESK_STORE_ADDR_CONST_SET(ret, NEG);
	/* - zero = zero */
	if(CESK_STORE_ADDR_CONST_CONTAIN(ret, ZERO))
		ret = CESK_STORE_ADDR_CONST_SET(ret, ZERO);
	if(ret == CESK_STORE_ADDR_CONST_PREFIX)
	{
		LOG_WARNING("an empty constant returned, the input address might be wrong ( which is %x)", a);
	}
	return ret;
}
/** @brief returns address for a+b 
 *  @param a first operand
 *  @param b second operand
 *  @return result address
 */
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
/** @brief returns address for a-b 
 *  @param a first operand
 *  @param b second operand
 *  @return result address
 */
static inline uint32_t cesk_addr_arithmetic_sub(uint32_t a, uint32_t b)
{
	/* a - b = a + (-b) */
	return cesk_addr_arithmetic_add(a, cesk_addr_arithmetic_neg(b));
}
/** @brief returns address for a*b 
 *  @param a first operand
 *  @param b second operand
 *  @return result address
 */
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
/** @brief returns address for a/b 
 *  @param a first operand
 *  @param b second operand
 *  @return result address
 */
static inline uint32_t cesk_addr_arithmetic_div(uint32_t a, uint32_t b)
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
/** @brief returns address for !a 
 *  @param a first operand
 *  @return result address
 */
static inline uint32_t cesk_addr_arithmetic_not(uint32_t a)
{
	uint32_t ret = CESK_STORE_ADDR_CONST_PREFIX;
	if(CESK_STORE_ADDR_CONST_CONTAIN(a, TRUE)) 
		ret = CESK_STORE_ADDR_CONST_SET(ret, FALSE);
	if(CESK_STORE_ADDR_CONST_CONTAIN(a, FALSE)) 
		ret = CESK_STORE_ADDR_CONST_SET(ret, TRUE);
	return ret;
}
/** @brief returns address for a&&b 
 *  @param a first operand
 *  @param b second operand
 *  @return result address
 */
static inline uint32_t cesk_addr_arithmetic_and(uint32_t a, uint32_t b)
{
	uint32_t ret = CESK_STORE_ADDR_CONST_PREFIX;
	if(CESK_STORE_ADDR_CONST_CONTAIN(a, TRUE) &&
	   CESK_STORE_ADDR_CONST_CONTAIN(b, TRUE))
		ret = CESK_STORE_ADDR_CONST_SET(ret, TRUE);
	if(CESK_STORE_ADDR_CONST_CONTAIN(a, TRUE) &&
	   CESK_STORE_ADDR_CONST_CONTAIN(b, FALSE))
		ret = CESK_STORE_ADDR_CONST_SET(ret, FALSE);
	if(CESK_STORE_ADDR_CONST_CONTAIN(a, FALSE) &&
	   CESK_STORE_ADDR_CONST_CONTAIN(b, TRUE))
		ret = CESK_STORE_ADDR_CONST_SET(ret, FALSE);
	if(CESK_STORE_ADDR_CONST_CONTAIN(a, FALSE) &&
	   CESK_STORE_ADDR_CONST_CONTAIN(b, FALSE))
		ret = CESK_STORE_ADDR_CONST_SET(ret, FALSE);
	if(CESK_STORE_ADDR_CONST_PREFIX == ret)
	{
		LOG_WARNING("empty set, type might be mismatch");
	}
	return ret;
}
/** @brief returns address for a||b 
 *  @param a first operand
 *  @param b second operand
 *  @return result address
 */
static inline uint32_t cesk_addr_arithmetic_or(uint32_t a, uint32_t b)
{
	uint32_t ret = CESK_STORE_ADDR_CONST_PREFIX;
	if(CESK_STORE_ADDR_CONST_CONTAIN(a, TRUE) &&
	   CESK_STORE_ADDR_CONST_CONTAIN(b, TRUE))
		ret = CESK_STORE_ADDR_CONST_SET(ret, TRUE);
	if(CESK_STORE_ADDR_CONST_CONTAIN(a, TRUE) &&
	   CESK_STORE_ADDR_CONST_CONTAIN(b, FALSE))
		ret = CESK_STORE_ADDR_CONST_SET(ret, TRUE);
	if(CESK_STORE_ADDR_CONST_CONTAIN(a, FALSE) &&
	   CESK_STORE_ADDR_CONST_CONTAIN(b, TRUE))
		ret = CESK_STORE_ADDR_CONST_SET(ret, TRUE);
	if(CESK_STORE_ADDR_CONST_CONTAIN(a, FALSE) &&
	   CESK_STORE_ADDR_CONST_CONTAIN(b, FALSE))
		ret = CESK_STORE_ADDR_CONST_SET(ret, FALSE);
	if(CESK_STORE_ADDR_CONST_PREFIX == ret)
	{
		LOG_WARNING("empty set, type might be mismatch");
	}
	return ret;
}
/** @brief returns address for a^b 
 *  @param a first operand
 *  @param b second operand
 *  @return result address
 */
static inline uint32_t cesk_addr_arithmetic_xor(uint32_t a, uint32_t b)
{
	uint32_t ret = CESK_STORE_ADDR_CONST_PREFIX;
	if(CESK_STORE_ADDR_CONST_CONTAIN(a, TRUE) &&
	   CESK_STORE_ADDR_CONST_CONTAIN(b, TRUE))
		ret = CESK_STORE_ADDR_CONST_SET(ret, FALSE);
	if(CESK_STORE_ADDR_CONST_CONTAIN(a, TRUE) &&
	   CESK_STORE_ADDR_CONST_CONTAIN(b, FALSE))
		ret = CESK_STORE_ADDR_CONST_SET(ret, TRUE);
	if(CESK_STORE_ADDR_CONST_CONTAIN(a, FALSE) &&
	   CESK_STORE_ADDR_CONST_CONTAIN(b, TRUE))
		ret = CESK_STORE_ADDR_CONST_SET(ret, TRUE);
	if(CESK_STORE_ADDR_CONST_CONTAIN(a, FALSE) &&
	   CESK_STORE_ADDR_CONST_CONTAIN(b, FALSE))
		ret = CESK_STORE_ADDR_CONST_SET(ret, FALSE);
	if(CESK_STORE_ADDR_CONST_PREFIX == ret)
	{
		LOG_WARNING("empty set, type might be mismatch");
	}
	return ret;
}
/** @brief returns address for a>b 
 *  @param a first operand
 *  @param b second operand
 *  @return result address
 */
static inline uint32_t cesk_addr_arithmetic_gt(uint32_t a, uint32_t b)
{
	int ret = CESK_STORE_ADDR_CONST_PREFIX;
	uint32_t tmp = cesk_addr_arithmetic_sub(a,b);
	if(CESK_STORE_ADDR_CONST_SET(tmp, NEG))
		ret = CESK_STORE_ADDR_CONST_SET(ret, FALSE);
	if(CESK_STORE_ADDR_CONST_SET(tmp, ZERO))
		ret = CESK_STORE_ADDR_CONST_CONTAIN(ret, FALSE);
	if(CESK_STORE_ADDR_CONST_CONTAIN(tmp, POS))
		ret = CESK_STORE_ADDR_CONST_SET(ret, TRUE);
	if(CESK_STORE_ADDR_CONST_PREFIX == ret)
	{
		LOG_WARNING("can not compare two value");
	}
	return ret;
}
/** @brief returns address for a=b 
 *  @param a first operand
 *  @param b second operand
 *  @return result address
 */
static inline uint32_t cesk_addr_arithmetic_eq(uint32_t a, uint32_t b)
{
	int ret = CESK_STORE_ADDR_CONST_PREFIX;
#if 0
	/* because there are object address comaprasion which is also compiled as
	 * numberic comparsion, so we should handle this situation here 
	 * since the comparasion of object address always involves constant 0 and a 
	 * empty constant address
	 */
	if(((a == CESK_STORE_ADDR_ZERO) && (b == CESK_STORE_ADDR_ZERO || (!CESK_STORE_ADDR_IS_CONST(b)))) || 
	   ((b == CESK_STORE_ADDR_ZERO) && (a == CESK_STORE_ADDR_ZERO|| (!CESK_STORE_ADDR_IS_CONST(a)))))
	{
		/* just swap the zero to the first operator */
		if(a != CESK_STORE_ADDR_ZERO)
		{
			uint32_t c = a;
			a = b;
			b = c;
		}
		if(b == CESK_STORE_ADDR_EMPTY)
		{
			return CESK_STORE_ADDR_TRUE;
		}
		return CESK_STORE_ADDR_FALSE;
	}
	/* if we use 0 as null pointer, things becomes easier */
#endif
	uint32_t tmp = cesk_addr_arithmetic_sub(a,b);
	if(CESK_STORE_ADDR_CONST_SET(tmp, NEG))
		ret = CESK_STORE_ADDR_CONST_SET(ret, FALSE);
	if(CESK_STORE_ADDR_CONST_SET(tmp, ZERO))
		ret = CESK_STORE_ADDR_CONST_CONTAIN(ret, TRUE);
	if(CESK_STORE_ADDR_CONST_CONTAIN(tmp, POS))
		ret = CESK_STORE_ADDR_CONST_SET(ret, FALSE);
	if(CESK_STORE_ADDR_CONST_PREFIX == ret)
	{
		LOG_WARNING("can not compare two value");
	}
	return ret;
}
/** @brief returns address for a>=b 
 *  @param a first operand
 *  @param b second operand
 *  @return result address
 */
static inline uint32_t cesk_addr_arithmetic_ge(uint32_t a, uint32_t b)
{
	return cesk_addr_arithmetic_ge(a,b) | cesk_addr_arithmetic_eq(a,b);
}
/** @brief returns address for a<b 
 *  @param a first operand
 *  @param b second operand
 *  @return result address
 */
static inline uint32_t cesk_addr_arithmetic_lt(uint32_t a, uint32_t b)
{
	return cesk_addr_arithmetic_ge(b,a);
}
/** @brief returns address for a<=b 
 *  @param a first operand
 *  @param b second operand
 *  @return result address
 */
static inline uint32_t cesk_addr_arithmetic_le(uint32_t a, uint32_t b)
{
	return cesk_addr_arithmetic_ge(b,a);
}
/* a != b */
/** @brief returns address for a!=b 
 *  @param a first operand
 *  @param b second operand
 *  @return result address
 */
static inline uint32_t cesk_addr_arithmetic_neq(uint32_t a, uint32_t b)
{
	return cesk_addr_arithmetic_not(cesk_addr_arithmetic_eq(a,b));
}
/** @brief returns the sign bit of the numeric
 *  @param a
 *  @return boolean address 
 */
static inline uint32_t cesk_addr_arithmetic_sign_bit(uint32_t a)
{
	uint32_t sa = CESK_STORE_ADDR_CONST_PREFIX;
	if(CESK_STORE_ADDR_CONST_CONTAIN(a, NEG)) 
		sa = CESK_STORE_ADDR_CONST_SET(sa, TRUE);
	if(CESK_STORE_ADDR_CONST_CONTAIN(a, ZERO)) 
		sa = CESK_STORE_ADDR_CONST_SET(sa, FALSE);
	if(CESK_STORE_ADDR_CONST_CONTAIN(a, POS)) 
		sa = CESK_STORE_ADDR_CONST_SET(sa, FALSE);
	return sa;
}
/** @brief convert sign bit to numberic
 *  @param sr boolean value
 *  @return numeric result
 */
static inline uint32_t cesk_addr_arithmetic_sign_bit_to_numeric(uint32_t sr)
{
	uint32_t ret = CESK_STORE_ADDR_CONST_PREFIX;
	if(CESK_STORE_ADDR_CONST_CONTAIN(sr, TRUE))
		ret = CESK_STORE_ADDR_CONST_SET(sr, NEG);
	if(CESK_STORE_ADDR_CONST_CONTAIN(sr, FALSE))
	{
		ret = CESK_STORE_ADDR_CONST_SET(sr, POS);
		ret = CESK_STORE_ADDR_CONST_SET(sr, ZERO);
	}
	return ret;
}
/** @brief returns address for bitwise and a&b 
 *  @param a first operand
 *  @param b second operand
 *  @return result address
 */
static inline uint32_t cesk_addr_arithmetic_bitwise_and(uint32_t a, uint32_t b)
{
	int ret = CESK_STORE_ADDR_CONST_PREFIX;

	uint32_t sa = cesk_addr_arithmetic_sign_bit(a);
	uint32_t sb = cesk_addr_arithmetic_sign_bit(b);
	

	uint32_t sr = cesk_addr_arithmetic_and(sa, sb);

	ret = cesk_addr_arithmetic_sign_bit_to_numeric(sr);


	if(CESK_STORE_ADDR_CONST_PREFIX == ret)
	{
		LOG_WARNING("an empty constant set returnted, it might be a mistake");
		return CESK_STORE_ADDR_CONST_PREFIX;
	}
	return ret;
}
/** @brief returns address for bitwise or a|b 
 *  @param a first operand
 *  @param b second operand
 *  @return result address
 */
static inline uint32_t cesk_addr_arithmetic_bitwise_or(uint32_t a, uint32_t b)
{
	int ret = CESK_STORE_ADDR_CONST_PREFIX;

	uint32_t sa = cesk_addr_arithmetic_sign_bit(a);
	uint32_t sb = cesk_addr_arithmetic_sign_bit(b);
	

	uint32_t sr = cesk_addr_arithmetic_or(sa, sb);

	ret = cesk_addr_arithmetic_sign_bit_to_numeric(sr);


	if(CESK_STORE_ADDR_CONST_PREFIX == ret)
	{
		LOG_WARNING("an empty constant set returnted, it might be a mistake");
		return CESK_STORE_ADDR_CONST_PREFIX;
	}
	return ret;
}
/** @brief returns address for bitwise xor a^b 
 *  @param a first operand
 *  @param b second operand
 *  @return result address
 */
static inline uint32_t cesk_addr_arithmetic_bitwise_xor(uint32_t a, uint32_t b)
{
	int ret = CESK_STORE_ADDR_CONST_PREFIX;

	uint32_t sa = cesk_addr_arithmetic_sign_bit(a);
	uint32_t sb = cesk_addr_arithmetic_sign_bit(b);
	

	uint32_t sr = cesk_addr_arithmetic_xor(sa, sb);

	ret = cesk_addr_arithmetic_sign_bit_to_numeric(sr);


	if(CESK_STORE_ADDR_CONST_PREFIX == ret)
	{
		LOG_WARNING("an empty constant set returnted, it might be a mistake");
		return CESK_STORE_ADDR_CONST_PREFIX;
	}
	return ret;
}
/** @brief return address for bitwise negative ~a
 *  @param a
 *  @return rsult
 */

static inline uint32_t cesk_addr_arithmetic_bitwise_neg(uint32_t a)
{
	int ret = CESK_STORE_ADDR_CONST_PREFIX;
	uint32_t sa = cesk_addr_arithmetic_sign_bit(a);
	uint32_t sr = cesk_addr_arithmetic_not(sa);
    ret = cesk_addr_arithmetic_sign_bit_to_numeric(sr);
	if(CESK_STORE_ADDR_CONST_PREFIX == ret)
	{
		LOG_WARNING("an empty constant set returnted, it might be a mistake");
		return CESK_STORE_ADDR_CONST_PREFIX;
	}
	return ret;
}
#endif
