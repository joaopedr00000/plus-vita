// SPDX-License-Identifier: AGPL-3.0-only
/* Exercise unchanged production arrival/sequence/queue/output code. Only the
 * external codec, device, clock and log writer are deterministic boundaries. */
#include "audio_playback_vita.h"
#include "stream_options.h"
#include "audio_diagnostic.h"
#include <opus.h>
#include <assert.h>
#include <pthread.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static pthread_mutex_t gate = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t changed = PTHREAD_COND_INITIALIZER;
static bool ready, finished, relative_pacing;
static PlusVitaAudioPlayback *audio;
static atomic_uint_fast64_t now_us;
static uint64_t next_arrival, output_deadline;
static unsigned sequence, output_count, log_delay, slow_output, initial_units;
static unsigned first_pcm_delay;
static unsigned native_block, expected_block, open_attempts, period_remainder;
static bool reject_small_block;
static unsigned options=PV_OPT_ALL; /* Exercise all experimental paths explicitly. */
static char final_timing[2048], final_summary[1024];
static PvAudioDiagnostic final_audio;
void plus_vita_diagnostic_audio(const PvAudioDiagnostic *s,bool final){if(final)final_audio=*s;}

uint64_t sceKernelGetProcessTimeWide(void) { return atomic_load(&now_us); }
static void deliver_until(uint64_t target)
{
	while(next_arrival <= target && output_count < 5000) {
		atomic_store(&now_us, next_arrival);
		uint8_t packet = 0;
		assert(plus_vita_audio_push(audio, (uint16_t)sequence++, &packet, 1));
		next_arrival += 10000;
	}
	atomic_store(&now_us, target);
}
OpusDecoder *opus_decoder_create(opus_int32 rate, int channels, int *error)
{
	assert(rate == 48000);
	pthread_mutex_lock(&gate);
	while(!ready) pthread_cond_wait(&changed, &gate);
	pthread_mutex_unlock(&gate);
	OpusDecoder *d = malloc(sizeof(*d)); assert(d); d->channels = channels; *error = OPUS_OK; return d;
}
OpusEncoder *opus_encoder_create(opus_int32 rate, int channels, int application, int *error)
{
	assert(rate == 48000 && application == OPUS_APPLICATION_AUDIO);
	OpusEncoder *e = malloc(sizeof(*e)); assert(e); e->channels = channels; *error = OPUS_OK; return e;
}
void opus_decoder_destroy(OpusDecoder *d) { free(d); }
void opus_encoder_destroy(OpusEncoder *e) { free(e); }
int opus_encode(OpusEncoder *e, const int16_t *pcm, int n, unsigned char *packet, opus_int32 cap)
{ (void)e; (void)pcm; assert(n == 480 && cap > 0); *packet = 0; return 1; }
int opus_decode(OpusDecoder *d, const unsigned char *packet, opus_int32 size, int16_t *pcm, int n, int fec)
{ (void)packet; (void)size; assert(n == 480 && !fec); memset(pcm, 0, 480 * d->channels * sizeof(*pcm)); return 480; }
int opus_decoder_ctl(OpusDecoder *d, int request, ...) { (void)d; assert(request == OPUS_RESET_STATE); return OPUS_OK; }
int sceAudioOutOpenPort(int type, int block, int rate, int mode)
{ assert(type == 0 && rate == 48000 && mode == 1); assert(block == (open_attempts++ ? 960 : (int)expected_block)); if(block == 512 && reject_small_block) return -1; native_block = (unsigned)block; return 1; }
int sceAudioOutSetVolume(int port, int flags, const int *v)
{ assert(port == 1 && flags == 3 && v[0] == 32768 && v[1] == 32768); return 0; }
int sceAudioOutGetConfig(int port, int key) { assert(port == 1); return key == 0 ? (int)native_block : key == 1 ? 48000 : 1; }
int sceAudioOutReleasePort(int port) { assert(port == 1); return 0; }
int sceAudioOutOutput(int port, const void *pcm)
{
	assert(port == 1); if(!pcm) return 0;
	uint64_t now = sceKernelGetProcessTimeWide();
	if(!output_count) output_deadline = now;
	/* Carry the fractional sample period (512/48000 s is not an integer in us). */
	unsigned scaled = native_block * 1000000u + period_remainder;
	unsigned period = scaled / 48000u;
	period_remainder = scaled % 48000u;
	output_deadline += period + slow_output;
	uint64_t target = relative_pacing ? now + period + slow_output : (now > output_deadline ? now : output_deadline);
	deliver_until(target);
	output_count++;
	if(output_count >= 5000) {
		pthread_mutex_lock(&gate); finished = true; pthread_cond_broadcast(&changed); pthread_mutex_unlock(&gate);
	}
	return 0;
}
static void log_message(ChiakiLogLevel level, const char *message, void *user)
{
	(void)level; (void)user;
	if(strstr(message, "audio timing final")) snprintf(final_timing, sizeof(final_timing), "%s", message);
	if(strstr(message, "audio summary")) snprintf(final_summary, sizeof(final_summary), "%s", message);
	if(log_delay && (strstr(message, "PCM levels decoded=") || strstr(message, "audio timing periodic")))
		deliver_until(sceKernelGetProcessTimeWide() + log_delay);
	if(first_pcm_delay && strstr(message, "audio first PCM decoded"))
		deliver_until(sceKernelGetProcessTimeWide() + first_pcm_delay);
}
static unsigned long long field(const char *line, const char *name)
{ const char *p = strstr(line, name); assert(p); return strtoull(p + strlen(name), NULL, 10); }
static uint64_t run(const char *name, unsigned startup, unsigned writer_delay, unsigned pacing_error, bool relative, bool after_first, unsigned profile)
{
	ready = finished = false; audio = NULL; sequence = output_count = 0;
	native_block = open_attempts = period_remainder = 0;
	expected_block = profile == 6000 && pv_opt(options,PV_OPT_AUDIO_SMALL) ? 512 : 960;
	log_delay = writer_delay; slow_output = pacing_error; relative_pacing = relative;
	first_pcm_delay = after_first ? startup : 0;
	if(after_first) startup = 0;
	initial_units = startup ? startup / 10000 : 2;
	atomic_store(&now_us, 1); final_timing[0] = final_summary[0] = 0;
	ChiakiLog log; chiaki_log_init(&log, CHIAKI_LOG_ALL, log_message, NULL);
	ChiakiAudioHeader header = { .channels = 2, .bits = 16, .rate = 48000, .frame_size = 480 };
	audio = plus_vita_audio_start_options(&header, &log,profile,options); assert(audio);
	/* Simulate arrivals during native initialization before its consumer runs. */
	for(unsigned i = 0; i < initial_units; i++) {
		atomic_store(&now_us, 1 + (startup ? i * 10000u : 0)); uint8_t packet = 0;
		assert(plus_vita_audio_push(audio, (uint16_t)sequence++, &packet, 1));
	}
	atomic_store(&now_us, 1 + startup);
	output_deadline = sceKernelGetProcessTimeWide(); next_arrival = output_deadline + 10000;
	pthread_mutex_lock(&gate); ready = true; pthread_cond_broadcast(&changed);
	while(!finished) pthread_cond_wait(&changed, &gate);
	pthread_mutex_unlock(&gate);
	assert(plus_vita_audio_stop(audio));
	assert(native_block == (reject_small_block && expected_block == 512 ? 960 : expected_block));
	assert(open_attempts == (reject_small_block && expected_block == 512 ? 2u : 1u));
	uint64_t average = field(final_timing, "queue_age_avg_us=");
	printf("%s: queue_avg_us=%llu queue_max_us=%llu gap_stalls=%llu drops=%llu outputs=%llu\n", name,
		(unsigned long long)average, field(final_timing, "queue_age_max_us="), field(final_timing, "output_gap_stalls="),
		field(final_summary, "dropped="), field(final_summary, "outputs="));
	assert(field(final_summary, "failed=") == 0);
    assert(final_audio.outputs==output_count&&final_audio.arrived>0&&!final_audio.failed&&final_audio.block_frames==native_block);
    assert(final_audio.queue_age_avg_us==average&&final_audio.dropped==field(final_summary,"dropped="));
	return average;
}
int main(void)
{
	setvbuf(stdout, NULL, _IONBF, 0);
	uint64_t clean = run("no startup / no writer delay", 0, 0, 0, false, false, 0);
	uint64_t startup = run("200ms initialization backlog", 200000, 0, 0, false, false, 0);
	uint64_t early = run("200ms writer stall after first dequeue", 200000, 0, 0, false, true, 0);
	uint64_t logging = run("startup + 2x10ms periodic writes / deadline pacing", 200000, 10000, 0, false, false, 0);
	uint64_t relative = run("startup + writes / fixed relative wait", 200000, 10000, 0, true, false, 0);
	uint64_t mismatch = run("startup + 20us output period mismatch", 200000, 0, 20, false, false, 0);
	assert(clean < 15000);
	assert(startup > 175000 && startup < 205000);
	assert(early > 175000 && early < 205000);
	/* The first periodic pair precedes the first hardware output and can
	 * establish an extra 20ms of backlog; later pairs produce transient spikes. */
	assert(logging >= startup && logging - startup < 25000);
	assert(relative > logging + 30000);
	assert(mismatch > startup + 25000);
	uint64_t low=run("adaptive 6 Mbps / 200ms startup",200000,0,0,false,false,6000);
	assert(low<160000 && field(final_summary,"queue_peak=")<=14 && field(final_summary,"dropped=")>0);
	reject_small_block=true;
	uint64_t fallback=run("adaptive 6 Mbps / rejected 512-frame port fallback",200000,0,0,false,false,6000);
	assert(fallback<160000 && field(final_summary,"queue_peak=")<=14 && field(final_summary,"dropped=")>0);
	reject_small_block=false;
	uint64_t quality=run("adaptive 15 Mbps / 200ms startup",200000,0,0,false,false,15000);
	assert(quality<180000 && field(final_summary,"queue_peak=")<=16 && field(final_summary,"dropped=")>0);
	options=PV_OPT_DEFAULT;
    uint64_t large=run("6 Mbps / small audio blocks disabled",200000,0,0,false,false,6000);
    assert(large<170000 && native_block==960 && final_audio.trimmed_pcm_samples==0 && final_audio.queue_peak<=15);
    options=0;
    uint64_t fixed=run("6 Mbps / all optional audio optimizations disabled",200000,0,0,false,false,6000);
    assert(fixed<170000 && native_block==960 && final_audio.trimmed_pcm_samples==0 && field(final_summary,"queue_peak=")<=15);
    puts("audio queue: legacy timing, adaptive backlog and opt-out paths passed; no hardware latency inferred");
}
