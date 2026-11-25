// SPDX-License-Identifier: AGPL-3.0-only
#include "pv_embedded_ca.h"
#include <mbedtls/x509_crt.h>
#include <assert.h>
#include <string.h>
#include <stdio.h>
int main(void)
{
	mbedtls_x509_crt chain;
	mbedtls_x509_crt_init(&chain);
	assert(strlen(pv_ca_pem)+1==sizeof(pv_ca_pem));
	int rc=mbedtls_x509_crt_parse(&chain,(const unsigned char *)pv_ca_pem,sizeof(pv_ca_pem));
	assert(rc==0);
	unsigned count=0;
	for(const mbedtls_x509_crt *p=&chain;p&&p->raw.p;p=p->next)count++;
	assert(count==PV_CA_CERT_COUNT);
	printf("Embedded CA: %u certificates parsed successfully\n",count);
	mbedtls_x509_crt_free(&chain);
	return 0;
}
