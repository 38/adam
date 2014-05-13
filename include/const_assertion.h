/** @brief the constant assertion utils, if assertion fails, a compilation error rises */
#ifndef __CONST_ASSERTION_H__
#define __CONST_ASSERTION_H__
/** @brief this macro is used to check if the constant is less than a given value, 
 *         if the assertion fails, this macro will rise a compilation error
 */
#define CONST_ASSERTION_LE(id,val) struct __const_checker_##id { int test[val - id];};
/** @brief this macro is used to check if the constant is greater than a given value, 
 *         if the assertion fails, this macro will rise a compilation error
 */
#define CONST_ASSERTION_GE(id,val) struct __const_checker_##id { int test[id - val];};
/** @brief this macro is used to check if the constant is less than a given value, 
 *         if the assertion fails, this macro will rise a compilation error
 */
#define CONST_ASSERTION_LT(id,val) struct __const_checker_##id { int test[val - id - 1];};
/** @brief this macro is used to check if the constant is greater than a given value, 
 *         if the assertion fails, this macro will rise a compilation error
 */
#define CONST_ASSERTION_GT(id,val) struct __const_checker_##id { int test[id - val - 1];};
/** @brief this macro is used to check if the constant is greater than a given value, 
 *         if the assertion fails, this macro will rise a compilation error
 */
#define CONST_ASSERTION_EQ(id,val) struct __const_checker_##id { int test[(id - val) * (val - id)];};

#endif
