// SPDX-License-Identifier: AGPL-3.0-only
#include "../src/platform/cloud_experimental.h"
#include <assert.h>
int main(void)
{
	assert(!plus_vita_manual_dc_valid(NULL));
	assert(!plus_vita_manual_dc_valid(""));
	assert(!plus_vita_manual_dc_valid("Auto"));
	assert(!plus_vita_manual_dc_valid(PLUS_VITA_DC_DISCOVER));
	assert(!plus_vita_manual_dc_valid("server\nname"));
	assert(plus_vita_manual_dc_valid("example-dc_1"));
	assert(!plus_vita_cloud_identifier_valid("short"));
	assert(plus_vita_cloud_identifier_valid("EXAMPLE-ENTITLEMENT"));
	assert(!plus_vita_bitrate_valid(0));
	assert(!plus_vita_bitrate_valid(16000));
	assert(plus_vita_next_bitrate(4000)==8000);
	assert(plus_vita_next_bitrate(8000)==12000);
	assert(plus_vita_next_bitrate(12000)==15000);
	assert(plus_vita_next_bitrate(15000)==4000);
	return 0;
}
