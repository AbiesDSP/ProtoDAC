#pragma once

// Monitor for unpleasant error conditions and mute if so.
// If the processor resets, this will fail to reset the
// earsaver timer, and the audio output will be muted.
void EarSaver(void *pvParameters);
void ear_saver_mute(void);
void ear_saver_unmute(void);
