#pragma once

/* Process data from the sync counter and update the sample rate feedback.
 * Monitor the state of the audio transmit buffer to determine if extra correction is needed.
 *
 */
void sync_init(void);
void SyncMonitor(void *AudioFBTask);
