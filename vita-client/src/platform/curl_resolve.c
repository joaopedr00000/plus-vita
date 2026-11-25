// SPDX-License-Identifier: AGPL-3.0-only

#include "curl_resolve.h"
#include "pv_embedded_ca.h"

#include <arpa/inet.h>
#include <netdb.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
 * VitaSDK's threaded libcurl resolver can leave pthread-embedded with an
 * invalid handle and crash while curl tears down its Happy Eyeballs filter.
 * Resolve the known HTTPS endpoint synchronously and seed CURLOPT_RESOLVE so
 * libcurl never starts that resolver thread. TLS still uses the original host
 * name for SNI and certificate validation.
 */
CURLcode plus_vita_curl_prepare_resolve(CURL *curl, const char *url,
	struct curl_slist **resolve_list)
{
	if(!curl || !url || !resolve_list)
		return CURLE_BAD_FUNCTION_ARGUMENT;
	*resolve_list = NULL;

	const char *scheme_end = strstr(url, "://");
	if(!scheme_end)
		return CURLE_URL_MALFORMAT;
	const char *host_begin = scheme_end + 3;
	const char *host_end = host_begin;
	while(*host_end && *host_end != ':' && *host_end != '/' &&
		*host_end != '?' && *host_end != '#')
		host_end++;
	if(host_end == host_begin || *host_begin == '[')
		return CURLE_URL_MALFORMAT;

	size_t host_size = (size_t)(host_end - host_begin);
	char *host = malloc(host_size + 1);
	if(!host)
		return CURLE_OUT_OF_MEMORY;
	memcpy(host, host_begin, host_size);
	host[host_size] = 0;

	unsigned int port = strncmp(url, "https://", 8) == 0 ? 443U : 80U;
	if(*host_end == ':')
	{
		char *port_end = NULL;
		unsigned long parsed = strtoul(host_end + 1, &port_end, 10);
		if(port_end == host_end + 1 || parsed == 0 || parsed > 65535)
		{
			free(host);
			return CURLE_URL_MALFORMAT;
		}
		port = (unsigned int)parsed;
	}

	struct addrinfo hints;
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	struct addrinfo *addresses = NULL;
	int gai_result = getaddrinfo(host, NULL, &hints, &addresses);
	if(gai_result != 0 || !addresses)
	{
		if(addresses)
			freeaddrinfo(addresses);
		free(host);
		return CURLE_COULDNT_RESOLVE_HOST;
	}

	char address[INET_ADDRSTRLEN * 8] = {0};
	size_t used=0; unsigned count=0;
	for(struct addrinfo *it=addresses;it&&count<8;it=it->ai_next){
		if(it->ai_family!=AF_INET||!it->ai_addr||it->ai_addrlen<sizeof(struct sockaddr_in))continue;
		char ip[INET_ADDRSTRLEN];
		struct sockaddr_in *resolved=(struct sockaddr_in *)it->ai_addr;
		if(!inet_ntop(AF_INET,&resolved->sin_addr,ip,sizeof(ip)))continue;
		int n=snprintf(address+used,sizeof(address)-used,"%s%s",count?",":"",ip);
		if(n<0||(size_t)n>=sizeof(address)-used)break;
		used+=(size_t)n;count++;
	}
	bool converted=count>0;
	freeaddrinfo(addresses);
	if(!converted)
	{
		free(host);
		return CURLE_COULDNT_RESOLVE_HOST;
	}

	size_t entry_size = host_size + 1 + 5 + 1 + strlen(address) + 1;
	char *entry = malloc(entry_size);
	if(!entry)
	{
		free(host);
		return CURLE_OUT_OF_MEMORY;
	}
	snprintf(entry, entry_size, "%s:%u:%s", host, port, address);
	free(host);

	*resolve_list = curl_slist_append(NULL, entry);
	free(entry);
	if(!*resolve_list)
		return CURLE_OUT_OF_MEMORY;

	CURLcode result = curl_easy_setopt(curl, CURLOPT_IPRESOLVE,
		CURL_IPRESOLVE_V4);
	if(result == CURLE_OK)
		result = curl_easy_setopt(curl, CURLOPT_RESOLVE, *resolve_list);
	if(result != CURLE_OK)
	{
		curl_slist_free_all(*resolve_list);
		*resolve_list = NULL;
	}
	return result;
}

/* Same public roots as the packaged PEM, directly in read-only memory.
 * Include the terminating NUL for mbedTLS PEM parsing. */
CURLcode plus_vita_curl_secure_options(CURL *curl)
{
	if(!curl)return CURLE_BAD_FUNCTION_ARGUMENT;
	CURLcode rc=curl_easy_setopt(curl,CURLOPT_SSL_VERIFYPEER,1L);
	if(rc==CURLE_OK)rc=curl_easy_setopt(curl,CURLOPT_SSL_VERIFYHOST,2L);
	if(rc==CURLE_OK)rc=curl_easy_setopt(curl,CURLOPT_SSLVERSION,(long)CURL_SSLVERSION_TLSv1_2);
	if(rc==CURLE_OK)rc=curl_easy_setopt(curl,CURLOPT_HTTP_VERSION,(long)CURL_HTTP_VERSION_1_1);
	if(rc!=CURLE_OK)return rc;
#if LIBCURL_VERSION_NUM >= 0x074d00
	struct curl_blob blob={.data=(void *)pv_ca_pem,.len=sizeof(pv_ca_pem),.flags=CURL_BLOB_NOCOPY};
	rc=curl_easy_setopt(curl,CURLOPT_CAINFO,NULL);
	if(rc==CURLE_OK)rc=curl_easy_setopt(curl,CURLOPT_CAPATH,NULL);
	if(rc==CURLE_OK)rc=curl_easy_setopt(curl,CURLOPT_CAINFO_BLOB,&blob);
	if(rc==CURLE_UNKNOWN_OPTION||rc==CURLE_NOT_BUILT_IN)
		rc=curl_easy_setopt(curl,CURLOPT_CAINFO,"app0:cacert.pem");
#else
	(void)pv_ca_pem;
	rc=curl_easy_setopt(curl,CURLOPT_CAINFO,"app0:cacert.pem");
#endif
	return rc;
}
