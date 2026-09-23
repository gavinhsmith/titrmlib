/* Host stand-in for CEdev's keypadc.h: just what titrmlib uses. kb_Scan()
 * plays back a script of keys set with stub_keys() (stubs.c): each key is
 * pressed on one scan and released on the next. */
#ifndef KEYPADC_H
#define KEYPADC_H

#include <stdint.h>

/* The keypad's data registers, one byte per group; titrmlib reads 1-7. */
extern uint8_t stub_kb_data[8];
#define kb_Data stub_kb_data

void kb_Scan(void);

/* Queues `n` scan codes (sk_* from ti/getcsc.h) to be pressed in turn. Once
 * they run out and titrmlib has handled every queued key, the next scan ends
 * term_run() with result STUB_IDLE_RESULT, unless stub_quit_when_idle is
 * cleared, so a test can't hang. */
#define STUB_IDLE_RESULT (-999)
extern int stub_quit_when_idle;
void stub_keys(const uint8_t *keys, int n);

/* Holds a key down across scans until stub_release(). */
void stub_hold(uint8_t sk);
void stub_release(void);

#endif
