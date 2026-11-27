// SPDX-License-Identifier: AGPL-3.0-only
#include "account_vita.h"
#include "../platform/request_cancel.h"
#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include <json_object.h>
#include <json_tokener.h>
#if PLUS_VITA_ACCOUNT_VAULT
#include "../platform/account_vault.h"
#include <chiaki/random.h>
#endif

#include "../platform/storage_paths.h"
#define IMPORT_FILE PV_IMPORT_FILE
#define ACCOUNT_DIR PV_ACCOUNT_DIR
#define ACCOUNT_FILE ACCOUNT_DIR "/account.json"
#define ACCOUNT_TEMP ACCOUNT_DIR "/account.tmp"
#define ACCOUNT_BLOCK ACCOUNT_DIR "/account.blocked"
#if PLUS_VITA_ACCOUNT_VAULT
/* Separate internal partition, not a fixed key or a key embedded in the JSON. */
#define KEY_DIR PV_KEY_DIR
#define KEY_FILE KEY_DIR "/account.key"
#define KEY_TEMP KEY_DIR "/account.key.tmp"
#endif
struct plus_vita_account_t {
	pthread_mutex_t mutex;
	pthread_t thread;
	bool running, complete, joined;
	PlusVitaAppEffect job;
	PlusVitaAppEvent result;
	PlusVitaAuthProbeResult catalogue;
	char file[4097];
};

static void clear_bytes(void *memory, size_t size)
{
	volatile unsigned char *p = memory;
	while(size--) *p++ = 0;
}

static void read_import(PlusVitaAccount *account)
{
	errno = 0;
	FILE *file = fopen(IMPORT_FILE, "rb");
	if(!file)
	{
		account->result.status = errno == ENOENT ? PLUS_VITA_RESULT_MISSING : PLUS_VITA_RESULT_STORAGE_ERROR;
		return;
	}
	size_t size = fread(account->file, 1, sizeof(account->file), file);
	bool failed = ferror(file) != 0;
	if(fclose(file) != 0) failed = true;
	/* The user-owned browser response stays in place for subsequent launches. */
	account->result.status = failed ? PLUS_VITA_RESULT_STORAGE_ERROR : PLUS_VITA_RESULT_OK;
	account->result.data = account->file;
	account->result.data_size = size;
}

static void authorize(PlusVitaAccount *account)
{
	#if PLUS_VITA_ADAPTIVE_STREAM
	plus_vita_auth_probe_region(&account->job.credential,&account->catalogue,account->job.region_mode);
#else
	plus_vita_auth_probe(&account->job.credential, &account->catalogue);
#endif
	PlusVitaAuthProbeResult *result = &account->catalogue;
	if(!result->transport_ok) account->result.status = PLUS_VITA_RESULT_NETWORK_ERROR;
	else if(!result->authorized) account->result.status = result->credential_rejected
		? PLUS_VITA_RESULT_REJECTED : PLUS_VITA_RESULT_CATALOGUE_ERROR;
	else if(!result->session_transport_ok) account->result.status = PLUS_VITA_RESULT_NETWORK_ERROR;
	else if(!result->session_created) account->result.status = PLUS_VITA_RESULT_CATALOGUE_ERROR;
	else if(!result->store_transport_ok) account->result.status = PLUS_VITA_RESULT_NETWORK_ERROR;
	else if((!result->store_resolved || !result->native_catalog_complete)&&!result->regional_catalog_complete) account->result.status = PLUS_VITA_RESULT_CATALOGUE_ERROR;
	else account->result.status = PLUS_VITA_RESULT_OK;
	account->result.auth_stage=result->transport_stage;
	account->result.auth_transport_error=result->transport_error;
	account->result.auth_curl_error=result->curl_error;
	account->result.regional_catalogue=result->regional_catalog_complete;
	account->result.library_complete=result->owned_authorized&&result->owned_complete;
	account->result.entries = result->native_titles;
	account->result.entry_count = result->native_titles_count;
	if(account->result.status == PLUS_VITA_RESULT_OK) {
		memcpy(account->result.account_name, result->online_id, sizeof(result->online_id));
		memcpy(account->result.store_country, result->store_country, sizeof(result->store_country));
		memcpy(account->result.store_language, result->store_language, sizeof(result->store_language));
		memcpy(account->result.language, result->language, sizeof(result->language));
	}
}

int plus_vita_account_file_kind(void)
{
	FILE *f=fopen(ACCOUNT_FILE,"rb");
	if(!f)return errno==ENOENT?0:-1;
	unsigned char prefix[4]={0};size_t n=fread(prefix,1,sizeof(prefix),f);
	bool bad=ferror(f)!=0;if(fclose(f)!=0)bad=true;
	if(bad)return -1;
#if PLUS_VITA_ACCOUNT_VAULT
	if(pv_vault_kind(prefix,n)==3)return 3;
#endif
	/* Only a JSON object can be a legacy file; damaged/unknown vaults fail closed. */
	if(n && (prefix[0]=='{'||prefix[0]==' '||prefix[0]=='\n'||prefix[0]=='\r'||prefix[0]=='\t'))return 1;
	return 2;
}
#if PLUS_VITA_ACCOUNT_VAULT
/* Remove only the known import path and only when its token matches the
 * successfully saved account. Never delete another account's import. */
static bool cleanup_matching_import(const PlusVitaCredential *credential)
{
    char raw[4097]={0};
    FILE *file=fopen(IMPORT_FILE,"rb");
    if(!file)return errno==ENOENT;
    size_t n=fread(raw,1,sizeof(raw)-1,file);
    int extra=fgetc(file);
    bool ok=!ferror(file)&&extra==EOF&&n>0&&!memchr(raw,0,n);
    if(fclose(file)!=0)ok=false;
    struct json_tokener *tok=ok?json_tokener_new():NULL;
    struct json_object *root=NULL,*token=NULL;
    if(tok){
        json_tokener_set_flags(tok,JSON_TOKENER_STRICT);
        root=json_tokener_parse_ex(tok,raw,(int)n+1);
        ok=json_tokener_get_error(tok)==json_tokener_success&&root&&
            json_object_is_type(root,json_type_object)&&
            json_object_object_get_ex(root,"npsso",&token)&&
            json_object_is_type(token,json_type_string)&&
            (size_t)json_object_get_string_len(token)==credential->length&&
            !memcmp(json_object_get_string(token),credential->value,credential->length);
        json_tokener_free(tok);
    }else ok=false;
    if(root)json_object_put(root);
    clear_bytes(raw,sizeof(raw));
    if(!ok)return false;
    return remove(IMPORT_FILE)==0||errno==ENOENT;
}
static bool key_read(uint8_t key[32]){
	FILE *f=fopen(KEY_FILE,"rb");if(!f)return false;
	size_t n=fread(key,1,32,f);int extra=fgetc(f);
	bool ok=n==32&&extra==EOF&&!ferror(f);if(fclose(f)!=0)ok=false;
	if(!ok)pv_vault_wipe(key,32);
	return ok;
}
static bool key_create(uint8_t key[32]){
	if(mkdir(KEY_DIR,0700)!=0&&errno!=EEXIST)return false;
	if(chiaki_random_bytes_crypt(key,32)!=CHIAKI_ERR_SUCCESS)return false;
	FILE *f=fopen(KEY_TEMP,"wb");bool ok=false;
	if(f){ok=fwrite(key,1,32,f)==32;if(fclose(f)!=0)ok=false;}
	if(ok)ok=rename(KEY_TEMP,KEY_FILE)==0;
	if(!ok)remove(KEY_TEMP);
	uint8_t check[32]={0};
	if(ok)ok=key_read(check)&&!memcmp(check,key,32);
	pv_vault_wipe(check,32);
	return ok;
}
static bool vault_read(FILE *file,const PlusVitaCredential *password,char *out,size_t cap,size_t *size)
{
	uint8_t sealed[PV_VAULT_MAX+PV_VAULT_OVERHEAD+1]={0};
	size_t n=fread(sealed,1,sizeof(sealed),file);
	uint8_t key[32]={0};bool automatic=pv_vault_kind(sealed,n)==3;
	bool ok=!ferror(file)&&(!automatic||key_read(key))&&
		pv_vault_open_impl(password->value,password->length,automatic?key:NULL,sealed,n,(uint8_t *)out,cap,size);
	pv_vault_wipe(key,32);
	pv_vault_wipe(sealed,sizeof(sealed));return ok;
}
#endif

static void load_account(PlusVitaAccount *account)
{
	struct stat status;
	if(stat(ACCOUNT_BLOCK, &status) == 0) { account->result.status = PLUS_VITA_RESULT_MISSING; account->result.restore_blocked = true; return; }
	if(errno != ENOENT) { account->result.status = PLUS_VITA_RESULT_STORAGE_ERROR; return; }
	FILE *file = fopen(ACCOUNT_FILE, "rb");
	if(!file) {
		#if PLUS_VITA_LAN_LOGIN
		if(errno == ENOENT) account->result.status=PLUS_VITA_RESULT_MISSING;
#else
		if(errno == ENOENT) { account->result.retained_json = true; read_import(account); }
#endif
		else account->result.status = PLUS_VITA_RESULT_STORAGE_ERROR;
		return;
	}
#if PLUS_VITA_ACCOUNT_VAULT
	int kind=plus_vita_account_file_kind();
	account->result.vault_present=kind>=2;account->result.vault_mode=kind>=2?(unsigned)kind:0;
	if(kind>=2){
		size_t size=0;bool ok=vault_read(file,&account->job.vault_password,account->file,sizeof(account->file),&size);
		if(fclose(file)!=0)ok=false;
		account->result.status=ok?PLUS_VITA_RESULT_OK:PLUS_VITA_RESULT_VAULT_LOCKED;
		if(ok){account->result.data=account->file;account->result.data_size=size;}
		return;
	}
	if(kind!=1){fclose(file);account->result.status=PLUS_VITA_RESULT_STORAGE_ERROR;return;}
	account->result.legacy_plain=true;
#endif
	size_t size = fread(account->file, 1, sizeof(account->file), file);
	bool failed = ferror(file) != 0;
	if(fclose(file) != 0) failed = true;
	account->result.status = failed ? PLUS_VITA_RESULT_STORAGE_ERROR : PLUS_VITA_RESULT_OK;
	account->result.data = account->file;
	account->result.data_size = size;
}

static bool account_directory(void)
{
	return mkdir(ACCOUNT_DIR, 0700) == 0 || errno == EEXIST;
}

static void save_account(PlusVitaAccount *account)
{
	account->result.status = PLUS_VITA_RESULT_STORAGE_ERROR;
	if(!account_directory()) return;
#if PLUS_VITA_ACCOUNT_VAULT
	int kind=plus_vita_account_file_kind();
	account->result.vault_present=kind>=2;account->result.vault_mode=kind>=2?(unsigned)kind:0;
	unsigned target=account->job.vault_target?account->job.vault_target:kind>=2?(unsigned)kind:3;
	if(kind<0||(target!=2&&target!=3)||(target==2&&!pv_vault_password(account->job.vault_password.value,account->job.vault_password.length)))return;
	if(kind>=2){
		FILE *old=fopen(ACCOUNT_FILE,"rb");
		if(!old)return;
		const PlusVitaCredential *previous=account->job.vault_change?&account->job.vault_previous:&account->job.vault_password;
		size_t old_size=0;bool unlocked=vault_read(old,previous,account->file,sizeof(account->file),&old_size);
		if(fclose(old)!=0)unlocked=false;
		clear_bytes(account->file,sizeof(account->file));
		if(!unlocked){account->result.status=PLUS_VITA_RESULT_VAULT_LOCKED;return;}
	}
#endif
#if PLUS_VITA_ACCOUNT_VAULT
	/* Only the non-secret label needs JSON escaping. Keep the token's serialized
	 * copy in a bounded, explicitly wiped buffer. */
	if(account->job.credential.length!=64)return;
	for(size_t j=0;j<64;j++){
		unsigned char ch=(unsigned char)account->job.credential.value[j];
		if(!((ch>='a'&&ch<='z')||(ch>='A'&&ch<='Z')||(ch>='0'&&ch<='9')))return;
	}
	char plain[1024]={0};
	struct json_object *root=json_object_new_string(account->job.label);
	if(!root)return;
	const char *quoted=json_object_to_json_string_ext(root,JSON_C_TO_STRING_PLAIN);
	int len=quoted?snprintf(plain,sizeof(plain),"{\"npsso\":\"%.64s\",\"label\":%s}",account->job.credential.value,quoted):-1;
	if(len<=0||(size_t)len>=sizeof(plain)){clear_bytes(plain,sizeof(plain));json_object_put(root);return;}
	const char *data=plain;
#else
	struct json_object *root = json_object_new_object();
	if(!root) return;
	struct json_object *token = json_object_new_string_len(account->job.credential.value, (int)account->job.credential.length);
	struct json_object *label = json_object_new_string(account->job.label);
	if(!token || !label) { json_object_put(token); json_object_put(label); json_object_put(root); return; }
	if(json_object_object_add(root, "npsso", token) < 0) {
		json_object_put(token); json_object_put(label); json_object_put(root); return;
	}
	if(json_object_object_add(root, "label", label) < 0) {
		json_object_put(label); json_object_put(root); return;
	}
	const char *data = json_object_to_json_string_ext(root, JSON_C_TO_STRING_PLAIN);
#endif
	bool success = false;
#if PLUS_VITA_ACCOUNT_VAULT
	uint8_t entropy[28]={0},sealed[PV_VAULT_MAX+PV_VAULT_OVERHEAD]={0},local_key[32]={0};
	size_t sealed_size=0;
	/* Reuse the key for an existing automatic vault; never replace it before
	 * committing a new ciphertext. A password vault remains recoverable if
	 * creation of the separate key or staged file fails. */
	if(data && (target!=3||(kind==3?key_read(local_key):key_create(local_key))) &&
		chiaki_random_bytes_crypt(entropy,sizeof(entropy))==CHIAKI_ERR_SUCCESS &&
		pv_vault_seal_impl(account->job.vault_password.value,account->job.vault_password.length,target==3?local_key:NULL,entropy,
			(const uint8_t *)data,strlen(data),sealed,sizeof(sealed),&sealed_size)){
		FILE *file=fopen(ACCOUNT_TEMP,"wb");
		if(file){
			success=fwrite(sealed,1,sealed_size,file)==sealed_size;
			if(fclose(file)!=0)success=false;
		}
		/* Authenticate the actual staged file before replacing the old account. */
		if(success){
			file=fopen(ACCOUNT_TEMP,"rb");size_t verified=0;
			success=file&&vault_read(file,&account->job.vault_password,account->file,sizeof(account->file),&verified);
			if(file&&fclose(file)!=0)success=false;
			if(success)success=verified==strlen(data)&&!memcmp(account->file,data,verified);
			clear_bytes(account->file,sizeof(account->file));
		}
	}
	pv_vault_wipe(entropy,sizeof(entropy));pv_vault_wipe(sealed,sizeof(sealed));pv_vault_wipe(local_key,sizeof(local_key));
#else
	FILE *file = fopen(ACCOUNT_TEMP, "wb");
	if(file)
	{
		success = data && fwrite(data, 1, strlen(data), file) == strlen(data);
		if(fclose(file) != 0) success = false;
	}
#endif
	if(success) success = rename(ACCOUNT_TEMP, ACCOUNT_FILE) == 0;
	if(success) success = remove(ACCOUNT_BLOCK) == 0 || errno == ENOENT;
#if PLUS_VITA_ACCOUNT_VAULT
    /* Verify the installed vault (including its separate key) before deleting
     * any plaintext source. A failure leaves the source untouched. */
    if(success){
        FILE *final=fopen(ACCOUNT_FILE,"rb");size_t verified=0;
        success=final&&vault_read(final,&account->job.vault_password,account->file,sizeof(account->file),&verified);
        if(final&&fclose(final)!=0)success=false;
        if(success)success=verified==strlen(data)&&!memcmp(account->file,data,verified);
        clear_bytes(account->file,sizeof(account->file));
    }
    pv_vault_wipe(plain,sizeof(plain));
#endif
	if(!success) remove(ACCOUNT_TEMP);
	json_object_put(root);
	if(success){
		account->result.status=PLUS_VITA_RESULT_OK;
#if PLUS_VITA_ACCOUNT_VAULT
		account->result.vault_present=true;account->result.vault_mode=target;
		if(target==2){remove(KEY_FILE);remove(KEY_TEMP);}
		if(!cleanup_matching_import(&account->job.credential))account->result.import_cleanup_failed=true;
#endif
	}
}

static void delete_account(PlusVitaAccount *account)
{
	account->result.status = PLUS_VITA_RESULT_STORAGE_ERROR;
	if(!account_directory()) return;
	/* A persistent tombstone prevents a failed unlink or old file from silently
	 * restoring an account the user has explicitly disabled/forgotten. */
	FILE *block = fopen(ACCOUNT_BLOCK, "wb");
	if(!block) return;
	if(fclose(block) != 0) return;
	account->result.restore_blocked = true;
	bool success = remove(ACCOUNT_FILE) == 0 || errno == ENOENT;
	if(remove(ACCOUNT_TEMP) != 0 && errno != ENOENT) success = false;
#if PLUS_VITA_ACCOUNT_VAULT
	if(remove(KEY_FILE)!=0&&errno!=ENOENT)success=false;
	if(remove(KEY_TEMP)!=0&&errno!=ENOENT)success=false;
#endif
	account->result.status = success ? PLUS_VITA_RESULT_OK : PLUS_VITA_RESULT_STORAGE_ERROR;
}

static void *worker(void *context)
{
	PlusVitaAccount *account = context;
	account->result.operation = account->job.operation;
	if(account->job.kind == PLUS_VITA_EFFECT_READ_IMPORT)
	{
		account->result.kind = PLUS_VITA_EVENT_IMPORT_READ;
		read_import(account);
	}
	else if(account->job.kind == PLUS_VITA_EFFECT_LOAD_ACCOUNT)
	{
		account->result.kind = PLUS_VITA_EVENT_ACCOUNT_LOADED;
		load_account(account);
	}
	else if(account->job.kind == PLUS_VITA_EFFECT_SAVE_ACCOUNT)
	{
		account->result.kind = PLUS_VITA_EVENT_ACCOUNT_SAVED;
		save_account(account);
	}
	else if(account->job.kind == PLUS_VITA_EFFECT_DELETE_ACCOUNT)
	{
		account->result.kind = PLUS_VITA_EVENT_ACCOUNT_DELETED;
		delete_account(account);
	}
	else if(account->job.kind == PLUS_VITA_EFFECT_ENABLE_AUTO)
	{
		account->result.kind = PLUS_VITA_EVENT_AUTO_ENABLED;
		account->result.status = remove(ACCOUNT_BLOCK) == 0 || errno == ENOENT
			? PLUS_VITA_RESULT_OK : PLUS_VITA_RESULT_STORAGE_ERROR;
	}
	else
	{
		account->result.kind = PLUS_VITA_EVENT_AUTHORIZED;
		authorize(account);
	}
	clear_bytes(&account->job, sizeof(account->job));
	pthread_mutex_lock(&account->mutex);
	account->complete = true;
	pthread_mutex_unlock(&account->mutex);
	return NULL;
}

PlusVitaAccount *plus_vita_account_create(void)
{
	PlusVitaAccount *account = calloc(1, sizeof(*account));
	if(!account) return NULL;
	if(pthread_mutex_init(&account->mutex, NULL) != 0) { free(account); return NULL; }
	return account;
}

bool plus_vita_account_start(PlusVitaAccount *account, const PlusVitaAppEffect *effect)
{
	if(account->running) return false;
	if(effect->kind != PLUS_VITA_EFFECT_READ_IMPORT
		&& effect->kind != PLUS_VITA_EFFECT_AUTHORIZE && effect->kind != PLUS_VITA_EFFECT_LOAD_ACCOUNT
		&& effect->kind != PLUS_VITA_EFFECT_SAVE_ACCOUNT && effect->kind != PLUS_VITA_EFFECT_DELETE_ACCOUNT
		&& effect->kind != PLUS_VITA_EFFECT_ENABLE_AUTO) return false;
	account->job = *effect;
	account->complete = account->joined = false;
	memset(&account->result, 0, sizeof(account->result));
	if(effect->kind == PLUS_VITA_EFFECT_AUTHORIZE) plus_vita_request_cancel_set(false);
	pthread_attr_t attr;
	bool attr_ready = pthread_attr_init(&attr) == 0;
	/* TLS and catalogue parsing share this worker stack. Reserve additional
	 * space only for authorization, not for storage jobs. */
	size_t stack_size=effect->kind==PLUS_VITA_EFFECT_AUTHORIZE?768u*1024u:256u*1024u;
	int result = attr_ready ? pthread_attr_setstacksize(&attr, stack_size) : -1;
	if(result == 0) result = pthread_create(&account->thread, &attr, worker, account);
	if(attr_ready) pthread_attr_destroy(&attr);
	if(result != 0) { clear_bytes(&account->job, sizeof(account->job)); return false; }
	account->running = true;
	return true;
}

bool plus_vita_account_idle(const PlusVitaAccount *account) { return !account->running; }

bool plus_vita_account_poll(PlusVitaAccount *account, PlusVitaAppEvent *event)
{
	if(!account->running || account->joined) return false;
	pthread_mutex_lock(&account->mutex);
	bool complete = account->complete;
	pthread_mutex_unlock(&account->mutex);
	if(!complete) return false;
	pthread_join(account->thread, NULL);
	account->joined = true;
	*event = account->result;
	return true;
}

void plus_vita_account_release_result(PlusVitaAccount *account)
{
	if(!account->joined) return;
	plus_vita_auth_probe_result_fini(&account->catalogue);
	memset(&account->catalogue, 0, sizeof(account->catalogue));
	clear_bytes(account->file, sizeof(account->file));
	memset(&account->result, 0, sizeof(account->result));
	account->running = false;
}

void plus_vita_account_cancel(PlusVitaAccount *account)
{
	(void)account;
	plus_vita_request_cancel_set(true);
}

bool plus_vita_account_close(PlusVitaAccount *account)
{
	if(!account) return true;
	plus_vita_account_cancel(account);
	if(account->running)
	{
		PlusVitaAppEvent ignored;
		if(!account->joined && !plus_vita_account_poll(account, &ignored)) return false;
		plus_vita_account_release_result(account);
	}
	pthread_mutex_destroy(&account->mutex);
	clear_bytes(account, sizeof(*account));
	free(account);
	return true;
}
