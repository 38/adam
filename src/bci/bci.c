#include <bci/bci.h>
int bci_init()
{
	if(bci_nametab_init() < 0)
	{
		LOG_FATAL("can not intialize BCI name table");
		return -1;
	}
	if(bci_package_init() < 0)
	{
		LOG_FATAL("can not initlaize BCI package libaray");
		return -1;
	}
	return 0;
}
void bci_finalize()
{
	bci_package_finalize();
	bci_nametab_finialize();
}
