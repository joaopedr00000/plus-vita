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
    assert(!vp_cloud_session_ids(j,cloud,stream)&&!strcmp(cloud,"original"));json_object_put(j);
    j=json_tokener_parse("{\"gkClientId\":\"12345678-1234-1234-1234-123456789abc\"}");
    assert(vp_cloud_session_ids(j,cloud,stream)&&!strcmp(cloud,"12345678-1234-1234-1234-123456789abc")&&!strcmp(stream,"original2"));json_object_put(j);
    assert(vp_cloud_event("{\"errors\":[{\"eventCode\":\"002.0011\",\"secret\":\"private\"}]}")==1);
    assert(vp_cloud_event("{\"name\":\"notAuthorized\",\"reason\":\"deviceNotActivated\"}")==9);
    assert(vp_cloud_event("{\"secret\":\"002.0011\"}")==0);
    assert(vp_cloud_event("invalid")==0);
    puts("Cloud response envelope, server-issued IDs and bounded error categories passed");return 0;
}
