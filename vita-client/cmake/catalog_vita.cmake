# SPDX-License-Identifier: AGPL-3.0-only
# Vita-only lower-memory adapter. Keep the pinned upstream eligibility/routing logic.
function(pv_catalog_replace variable old new)
  string(FIND "${${variable}}" "${old}" position)
  if(position EQUAL -1)
    message(FATAL_ERROR "Pinned catalogue patch anchor missing: ${variable}")
  endif()
  string(REPLACE "${old}" "${new}" updated "${${variable}}")
  set(${variable} "${updated}" PARENT_SCOPE)
endfunction()
file(READ "${PYLUX_ROOT}/lib/src/cloudcatalog_unified.c" PV_CATALOG_UNIFIED_TEXT)
pv_catalog_replace(PV_CATALOG_UNIFIED_TEXT [==[#include "cloudcatalog_internal.h"]==] [==[#include "cloudcatalog_internal.h"
#include "diagnostic_log.h"]==])
pv_catalog_replace(PV_CATALOG_UNIFIED_TEXT [==[static ChiakiErrorCode finish_ok(ChiakiCloudCatalogResult *out, struct json_object *env)
{]==] [==[static ChiakiErrorCode finish_ok(ChiakiCloudCatalogResult *out, struct json_object *env, struct json_object **direct)
{
	if(direct){
		*direct=json_object_get(env);
		out->err=*direct?CHIAKI_ERR_SUCCESS:CHIAKI_ERR_MEMORY;
		return out->err;
	}]==])
pv_catalog_replace(PV_CATALOG_UNIFIED_TEXT [==[CHIAKI_EXPORT ChiakiErrorCode chiaki_cloudcatalog_fetch_unified(
	const ChiakiCloudCatalogConfig *config, ChiakiCloudCatalogResult *out, ChiakiLog *log)
{]==] [==[static ChiakiErrorCode pv_fetch_unified(
	const ChiakiCloudCatalogConfig *config, ChiakiCloudCatalogResult *out, ChiakiLog *log, struct json_object **direct)
{
	if(direct)*direct=NULL;]==])
pv_catalog_replace(PV_CATALOG_UNIFIED_TEXT [==[cc_cache_ensure_dir(cache_dir);]==] [==[if(!direct)cc_cache_ensure_dir(cache_dir);]==])
pv_catalog_replace(PV_CATALOG_UNIFIED_TEXT [==[finish_ok(out, cached)]==] [==[finish_ok(out, cached, direct)]==])
pv_catalog_replace(PV_CATALOG_UNIFIED_TEXT [==[finish_ok(out, env)]==] [==[finish_ok(out, env, direct)]==])
pv_catalog_replace(PV_CATALOG_UNIFIED_TEXT [==[CCNativeResult nr = cc_fetch_psnow_native]==] [==[plus_vita_diagnostic_event("auth.phase",8);
	CCNativeResult nr = cc_fetch_psnow_native]==])
pv_catalog_replace(PV_CATALOG_UNIFIED_TEXT [==[apollo = cc_fetch_apollo_fallback(log, cc, &fallback_complete);]==] [==[plus_vita_diagnostic_event("auth.phase",9);
		apollo = cc_fetch_apollo_fallback(log, cc, &fallback_complete);]==])
pv_catalog_replace(PV_CATALOG_UNIFIED_TEXT [==[// 3. imagic (cache, then network with locale fallback).]==] [==[plus_vita_diagnostic_event("auth.phase",10);
	// 3. imagic (cache, then network with locale fallback).]==])
pv_catalog_replace(PV_CATALOG_UNIFIED_TEXT [==[if(ir.all_ps5_list_succeeded)
				write_v6_cache]==] [==[if(!direct && ir.all_ps5_list_succeeded)
				write_v6_cache]==])
pv_catalog_replace(PV_CATALOG_UNIFIED_TEXT [==[// 4. owned entitlements (skip on missing/expired session).]==] [==[plus_vita_diagnostic_event("auth.phase",11);
	// 4. owned entitlements (skip on missing/expired session).]==])
pv_catalog_replace(PV_CATALOG_UNIFIED_TEXT [==[if(orr == CC_OWNED_OK)
				write_library_cache(log, cache_dir, owned, components);]==] [==[if(orr == CC_OWNED_OK){
				if(!direct)write_library_cache(log,cache_dir,owned,components);
			}]==])
pv_catalog_replace(PV_CATALOG_UNIFIED_TEXT [==[// 5. cross-reference owned -> catalog.]==] [==[plus_vita_diagnostic_event("auth.phase",12);
	// 5. cross-reference owned -> catalog.]==])
pv_catalog_replace(PV_CATALOG_UNIFIED_TEXT [==[// 6. assemble.]==] [==[plus_vita_diagnostic_event("auth.phase",13);
	// 6. assemble.]==])
pv_catalog_replace(PV_CATALOG_UNIFIED_TEXT [==[// 7. cache write guard]==] [==[plus_vita_diagnostic_event("auth.phase",14);
	// 7. cache write guard]==])
pv_catalog_replace(PV_CATALOG_UNIFIED_TEXT [==[if(total > 0 && !auth_error && apollo_complete && browse_complete && owned_complete)]==] [==[if(!direct && total > 0 && !auth_error && apollo_complete && browse_complete && owned_complete)]==])
pv_catalog_replace(PV_CATALOG_UNIFIED_TEXT [==[	ChiakiErrorCode e = finish_ok(out, env, direct);]==] [==[	plus_vita_diagnostic_event("auth.phase",15);
	ChiakiErrorCode e = finish_ok(out, env, direct);]==])
pv_catalog_replace(PV_CATALOG_UNIFIED_TEXT [==[CHIAKI_EXPORT void chiaki_cloudcatalog_result_fini(]==] [==[/* Preserve the public Pylux API; Vita uses a direct, in-memory result. */
CHIAKI_EXPORT ChiakiErrorCode chiaki_cloudcatalog_fetch_unified(
	const ChiakiCloudCatalogConfig *config, ChiakiCloudCatalogResult *out, ChiakiLog *log)
{
	return pv_fetch_unified(config,out,log,NULL);
}
ChiakiErrorCode plus_vita_catalog_fetch_direct(
	const ChiakiCloudCatalogConfig *config, ChiakiCloudCatalogResult *out, ChiakiLog *log, struct json_object **direct)
{
	if(!direct)return CHIAKI_ERR_INVALID_DATA;
	return pv_fetch_unified(config,out,log,direct);
}

CHIAKI_EXPORT void chiaki_cloudcatalog_result_fini(]==])
pv_catalog_replace(PV_CATALOG_UNIFIED_TEXT [==[	// 6. assemble.]==] [==[	/* Cross-reference returns independent rows. Release inputs no longer
	 * used by the assembly before allocating its indexes and row objects. */
	json_object_put(aliases);aliases=NULL;
	json_object_put(owned);owned=NULL;
	json_object_put(components);components=NULL;
	// 6. assemble.]==])
pv_catalog_replace(PV_CATALOG_UNIFIED_TEXT [==[	ChiakiErrorCode e = finish_ok(out, env, direct);]==] [==[	ChiakiErrorCode e;
	if(!env){out->err=CHIAKI_ERR_MEMORY;e=out->err;}
	else e=finish_ok(out,env,direct);]==])
set(PV_CATALOG_UNIFIED_SOURCE "${CMAKE_CURRENT_BINARY_DIR}/generated/cloudcatalog_unified_vita.c")
file(WRITE "${PV_CATALOG_UNIFIED_SOURCE}" "${PV_CATALOG_UNIFIED_TEXT}")
file(READ "${PYLUX_ROOT}/lib/src/cloudcatalog_fetch.c" PV_CATALOG_FETCH_TEXT)
pv_catalog_replace(PV_CATALOG_FETCH_TEXT [==[struct json_object *gc = cc_json_clone(v1);]==] [==[struct json_object *gc = json_object_get(v1);]==])
pv_catalog_replace(PV_CATALOG_FETCH_TEXT [==[struct json_object *gc = cc_json_clone(v2);]==] [==[struct json_object *gc = json_object_get(v2);]==])
pv_catalog_replace(PV_CATALOG_FETCH_TEXT [==[json_object_array_add(accumulated, cc_json_clone(json_object_array_get_idx(page, i)));]==] [==[json_object_array_add(accumulated, json_object_get(json_object_array_get_idx(page, i)));]==])
set(PV_CATALOG_FETCH_SOURCE "${CMAKE_CURRENT_BINARY_DIR}/generated/cloudcatalog_fetch_vita.c")
file(WRITE "${PV_CATALOG_FETCH_SOURCE}" "${PV_CATALOG_FETCH_TEXT}")
file(READ "${PYLUX_ROOT}/lib/src/cloudcatalog_merge.c" PV_CATALOG_MERGE_TEXT)
pv_catalog_replace(PV_CATALOG_MERGE_TEXT [==[json_object_array_add(universe, cc_json_clone(json_object_array_get_idx(apollo_norm, i)));]==] [==[json_object_array_add(universe, json_object_get(json_object_array_get_idx(apollo_norm, i)));]==])
pv_catalog_replace(PV_CATALOG_MERGE_TEXT [==[json_object_array_add(universe, cc_json_clone(json_object_array_get_idx(ps5_browse, i)));]==] [==[json_object_array_add(universe, json_object_get(json_object_array_get_idx(ps5_browse, i)));]==])
pv_catalog_replace(PV_CATALOG_MERGE_TEXT [==[struct json_object *games = merge_owned_into_browse(universe, in->owned_cross_ref, true);]==] [==[struct json_object *games = merge_owned_into_browse(universe, in->owned_cross_ref, true);
	json_object_put(universe);
	json_object_put(ps5_browse);]==])
pv_catalog_replace(PV_CATALOG_MERGE_TEXT [==[	json_object_put(ps5_browse);
	json_object_put(universe);
	return out;]==] [==[	return out;]==])
pv_catalog_replace(PV_CATALOG_MERGE_TEXT [==[#include "cloudcatalog_internal.h"]==] [==[#include "cloudcatalog_internal.h"
#include "diagnostic_log.h"
/* During assembly only, the nested values are read-only. Each row owns
 * its independent top-level fields, while sharing immutable nested JSON.
 * Never use this helper in the ownership cross-reference mutation stage. */
static struct json_object *pv_catalog_row_copy(struct json_object *src)
{
	if(!src || json_object_get_type(src)!=json_type_object)return NULL;
	struct json_object *out=json_object_new_object();
	if(!out)return NULL;
	json_object_object_foreach(src,key,value){
		struct json_object *ref=json_object_get(value);
		if(json_object_object_add(out,key,ref)!=0){
			json_object_put(ref);
			json_object_put(out);
			return NULL;
		}
	}
	return out;
}]==])
pv_catalog_replace(PV_CATALOG_MERGE_TEXT [==[struct json_object *g = cc_json_clone(raw);]==] [==[struct json_object *g = pv_catalog_row_copy(raw);]==])
pv_catalog_replace(PV_CATALOG_MERGE_TEXT [==[struct json_object *g = cc_json_clone(v);
			const char *existing]==] [==[struct json_object *g = pv_catalog_row_copy(v);
			if(!g){json_object_put(apollo_norm);json_object_put(apollo_pids);json_object_put(ps5_browse);return NULL;}
			const char *existing]==])
pv_catalog_replace(PV_CATALOG_MERGE_TEXT [==[		for(size_t i = 0; i < n; i++)
			json_object_array_add(games, cc_json_clone(json_object_array_get_idx(browse, i)));]==] [==[		for(size_t i = 0; i < n; i++){
			struct json_object *row=pv_catalog_row_copy(json_object_array_get_idx(browse,i));
			if(!row){json_object_put(games);return NULL;}
			if(json_object_array_add(games,row)!=0){json_object_put(row);json_object_put(games);return NULL;}
		}]==])
pv_catalog_replace(PV_CATALOG_MERGE_TEXT [==[struct json_object *entry = cc_json_clone(owned_game);]==] [==[struct json_object *entry = pv_catalog_row_copy(owned_game);
		if(!entry){
			json_object_put(fully_owned);json_object_put(ordered);
			json_object_put(ix.by_product);json_object_put(ix.by_concept);json_object_put(games);
			return NULL;
		}]==])
pv_catalog_replace(PV_CATALOG_MERGE_TEXT [==[	// 1. Normalize apollo rows + collect their productIds.]==] [==[	plus_vita_diagnostic_event("catalog.phase",1);
	// 1. Normalize apollo rows + collect their productIds.]==])
pv_catalog_replace(PV_CATALOG_MERGE_TEXT [==[	// 2. PS5 browse rows: device-based filter, skip apollo dups, stamp pscloud.]==] [==[	plus_vita_diagnostic_event("catalog.phase",2);
	// 2. PS5 browse rows: device-based filter, skip apollo dups, stamp pscloud.]==])
pv_catalog_replace(PV_CATALOG_MERGE_TEXT [==[	// 3. universe = apollo + ps5Browse]==] [==[	plus_vita_diagnostic_event("catalog.phase",3);
	// 3. universe = apollo + ps5Browse]==])
pv_catalog_replace(PV_CATALOG_MERGE_TEXT [==[	// 4. merge owned]==] [==[	plus_vita_diagnostic_event("catalog.phase",4);
	// 4. merge owned]==])
pv_catalog_replace(PV_CATALOG_MERGE_TEXT [==[	// 5. streamability gate (native mode only)]==] [==[	if(!games){json_object_put(apollo_norm);json_object_put(apollo_pids);return NULL;}
	plus_vita_diagnostic_event("catalog.phase",5);
	// 5. streamability gate (native mode only)]==])
pv_catalog_replace(PV_CATALOG_MERGE_TEXT [==[	// 6. tag every game with the full contract.]==] [==[	plus_vita_diagnostic_event("catalog.phase",6);
	// 6. tag every game with the full contract.]==])
pv_catalog_replace(PV_CATALOG_MERGE_TEXT [==[	struct json_object *deduped = dedupe_contract_product_ids(games);]==] [==[	plus_vita_diagnostic_event("catalog.phase",7);
	struct json_object *deduped = dedupe_contract_product_ids(games);]==])
pv_catalog_replace(PV_CATALOG_MERGE_TEXT [==[	// 7. envelope]==] [==[	plus_vita_diagnostic_event("catalog.phase",8);
	// 7. envelope]==])
pv_catalog_replace(PV_CATALOG_MERGE_TEXT [==[	json_object_put(apollo_pids);
	return out;]==] [==[	json_object_put(apollo_pids);
	plus_vita_diagnostic_event("catalog.phase",9);
	return out;]==])
set(PV_CATALOG_MERGE_SOURCE "${CMAKE_CURRENT_BINARY_DIR}/generated/cloudcatalog_merge_vita.c")
file(WRITE "${PV_CATALOG_MERGE_SOURCE}" "${PV_CATALOG_MERGE_TEXT}")
