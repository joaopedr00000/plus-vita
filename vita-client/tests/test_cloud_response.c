// SPDX-License-Identifier: AGPL-3.0-only
#include "../src/platform/cloud_response.h"
#include <assert.h>
#include <stdio.h>
int main(void){
    struct json_object *j=json_tokener_parse("{\"dataCenters\":[{\"dataCenter\":\"sample\"}]}");
    assert(vp_cloud_dc_array(j)&&json_object_array_length(vp_cloud_dc_array(j))==1);json_object_put(j);
    j=json_tokener_parse("{\"datacenters\":[]}");assert(vp_cloud_dc_array(j)&&json_object_array_length(vp_cloud_dc_array(j))==0);json_object_put(j);
    j=json_tokener_parse("{\"dataCenters\":[],\"datacenters\":[]}");assert(!vp_cloud_dc_array(j));json_object_put(j);
    j=json_tokener_parse("[]");assert(vp_cloud_dc_array(j)==j);json_object_put(j);
    j=json_tokener_parse("{\"dataCenterList\":42}");assert(!vp_cloud_dc_array(j));json_object_put(j);
    char cloud[128]="original",stream[128]="original2";
    j=json_tokener_parse("{\"gkClientId\":\"12345678-1234-1234-1234-123456789abc\",\"streamServerClientId\":\"invalid\"}");
    assert(vp_cloud_session_ids(j,cloud,stream)&&!strcmp(cloud,"12345678-1234-1234-1234-123456789abc")&&!strcmp(stream,"original2"));json_object_put(j);
    j=json_tokener_parse("{\"gkClientId\":\"12345678-1234-1234-1234-123456789abc\"}");
    assert(vp_cloud_session_ids(j,cloud,stream)&&!strcmp(cloud,"12345678-1234-1234-1234-123456789abc")&&!strcmp(stream,"original2"));json_object_put(j);
    assert(vp_cloud_event("{\"errors\":[{\"eventCode\":\"002.0011\",\"secret\":\"private\"}]}")==1);
    assert(vp_cloud_event("{\"name\":\"notAuthorized\",\"reason\":\"deviceNotActivated\"}")==9);
    assert(vp_cloud_event("{\"secret\":\"002.0011\"}")==0);
    assert(vp_cloud_event("invalid")==0);
    assert(vp_cloud_event("002.0011")==1);
    assert(vp_cloud_event("{\"errors\":[{\"name\":\"invalidAuthCode\"}]}")==12);
    assert(vp_cloud_event("{\"errors\":[{\"description\":\"Bad Request\"}]}")==19);
    assert(vp_cloud_event("{\"componentCode\":\"002\"}")==21);
    assert(vp_cloud_event("{\"description\":\"private arbitrary text\"}")==0);
    char *code=vp_cloud_oauth_code("gaikai://local?code=abc%2F%2B%3D%25+def&other=1#ignore");
    assert(code&&!strcmp(code,"abc/+=%+def"));free(code);
    code=vp_cloud_oauth_code("gaikai://local?code=abc%252F");assert(code&&!strcmp(code,"abc%2F"));free(code);
    assert(!vp_cloud_oauth_code("gaikai://local?code=x&code=y"));
    assert(!vp_cloud_oauth_code("gaikai://local?code=%00"));
    assert(!vp_cloud_oauth_code("gaikai://local?code=%0D%0A"));
    assert(!vp_cloud_oauth_code("gaikai://local?code=%ZZ"));
    assert(!vp_cloud_oauth_code("gaikai://local?code=%A"));
    assert(!vp_cloud_oauth_code("gaikai://local#?code=not-a-query"));
    assert(!vp_cloud_oauth_code("gaikai://local?code="));
    char *key=vp_cloud_response_key("header-key","{\"sessionKey\":\"body-key\"}");
    assert(key&&!strcmp(key,"header-key"));free(key);
    key=vp_cloud_response_key(NULL,"{\"sessionKey\":\"body-key\"}");
    assert(key&&!strcmp(key,"body-key"));free(key);
    assert(!vp_cloud_response_key("bad\r\nheader","{\"sessionKey\":\"body-key\"}"));
    assert(!vp_cloud_response_key(NULL,"{\"sessionKey\":\"bad\\u0000key\"}"));
    assert(!vp_cloud_response_key(NULL,"{\"secret\":\"not-a-session-key\"}"));
    assert(!vp_cloud_response_key(NULL,"{\"sessionKey\":123}"));
    puts("Cloud response envelope, server-issued IDs and bounded error categories passed");return 0;
}
