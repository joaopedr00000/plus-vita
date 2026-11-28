// SPDX-License-Identifier: AGPL-3.0-only
#ifndef PV_AUDIO_DIAGNOSTIC_H
#define PV_AUDIO_DIAGNOSTIC_H
#include <stdint.h>
#include <stdbool.h>
/* Numeric-only snapshots: never pass packet payloads, addresses or accounts. */
typedef struct {
    unsigned arrived,decoded,outputs,concealed,stale,dropped,queue_peak,queue_now;
    unsigned arrival_gt20ms,arrival_gt40ms,output_gap_stalls,output_call_stalls;
    unsigned seq_gaps,seq_resets,max_missing,skipped_local,empty_waits;
    unsigned refill_count,brief_waits,target_ms,block_frames,failed;
    uint64_t queue_age_avg_us,queue_age_max_us,arrival_gap_max_us,output_gap_max_us,output_call_max_us;
    uint64_t refill_wait_total_us,refill_wait_max_us,discarded_pcm_samples,trimmed_pcm_samples;
} PvAudioDiagnostic;
void plus_vita_diagnostic_audio(const PvAudioDiagnostic *stats,bool final);
#endif
