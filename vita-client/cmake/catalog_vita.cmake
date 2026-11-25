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
set(PV_CATALOG_MERGE_SOURCE "${CMAKE_CURRENT_BINARY_DIR}/generated/cloudcatalog_merge_vita.c")
file(WRITE "${PV_CATALOG_MERGE_SOURCE}" "${PV_CATALOG_MERGE_TEXT}")
