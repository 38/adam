#include <bci/bci_package.h>
/**
 * @brief the type for a buildin class package 
 * @note in the design, different pacakge implmements different version of APIs
 **/
typedef struct {
	struct {
		uint32_t maj;      /*!< the major version number */
		uint32_t min;      /*!< the minor version number */
	} api_version;         /*!< the api version this package provides */
} _bci_package_t;

static _bci_package_t packinfo;
