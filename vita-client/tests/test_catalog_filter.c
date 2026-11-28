// SPDX-License-Identifier: AGPL-3.0-only
#include "../src/app/catalog_filter.h"
#include <assert.h>
int main(void){
	assert(pv_catalog_matches(0,""));
	assert(pv_catalog_matches(1,"PS3"));
	assert(pv_catalog_matches(2,"ps4"));
	assert(pv_catalog_matches(3,"PS5"));
	assert(!pv_catalog_matches(1,"PS4"));
	assert(!pv_catalog_matches(3,""));
	assert(!pv_catalog_matches(3,"PS5x"));
	assert(!pv_catalog_matches(3,"P"));
	assert(pv_catalog_next(3)==4&&pv_catalog_next(4)==5&&pv_catalog_next(5)==0);
	assert(pv_catalog_entry_matches(4,"PS5",true)&&!pv_catalog_entry_matches(4,"PS5",false));
	assert(pv_catalog_entry_matches(1,"PS3",false)&&!pv_catalog_entry_matches(2,"PS3",true));
	assert(pv_catalog_matches(5,"PS2")&&pv_catalog_matches(5,"ps2")&&!pv_catalog_matches(5,"PS4"));
	return 0;
}
