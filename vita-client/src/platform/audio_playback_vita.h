#pragma once
#include <chiaki/audio.h>
#include <chiaki/log.h>
typedef struct plus_vita_audio_playback_t PlusVitaAudioPlayback;
PlusVitaAudioPlayback *plus_vita_audio_start(const ChiakiAudioHeader *header, ChiakiLog *log);
void plus_vita_audio_set_profile(PlusVitaAudioPlayback *audio,unsigned cap_kbps);
bool plus_vita_audio_push(PlusVitaAudioPlayback *audio, uint16_t sequence, const uint8_t *data, size_t size);
bool plus_vita_audio_stop(PlusVitaAudioPlayback *audio);
