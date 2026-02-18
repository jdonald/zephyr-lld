/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * "Shave and a Haircut, Two Bits" LED blink for Arduino Uno R4 Minima.
 *
 * Blinks the built-in LED (D13) in the classic 5-and-2 knock rhythm
 * once after each reset, then stays off.
 *
 * Rhythm (at ~120 BPM, quarter note = 500 ms):
 *
 *   "Shave  and-a  hair - cut"   [pause]   "two    bits"
 *    q      ee     q     q                   q      h
 *    1      2+     3     4        (rest)     1      2—3
 *
 *   q  = quarter note  (500 ms)
 *   e  = eighth note   (250 ms)
 *   h  = half note     (1000 ms) — held longer for the final "bits"
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>

#define LED0_NODE DT_ALIAS(led0)

static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(LED0_NODE, gpios);

/* Duration the LED stays ON for each "knock" (ms). */
#define KNOCK_MS 120

/* Timing helpers (ms). */
#define EIGHTH   250
#define QUARTER  500
#define HALF     1000

/*
 * The pattern is encoded as an array of gaps (LED-off time in ms)
 * that follow each knock.  There are 7 knocks total.
 *
 *   Knock 1  "Shave"    -> gap: quarter - knock
 *   Knock 2  "and"      -> gap: eighth  - knock  (quick)
 *   Knock 3  "a"        -> gap: eighth  - knock  (quick)
 *   Knock 4  "hair"     -> gap: quarter - knock
 *   Knock 5  "cut"      -> gap: quarter + half - knock  (dramatic pause)
 *   Knock 6  "two"      -> gap: quarter - knock
 *   Knock 7  "bits"     -> 0  (pattern done)
 */
static const int gaps[] = {
	QUARTER - KNOCK_MS,           /* after "Shave"  */
	EIGHTH  - KNOCK_MS,           /* after "and"    */
	EIGHTH  - KNOCK_MS,           /* after "a"      */
	QUARTER - KNOCK_MS,           /* after "hair"   */
	QUARTER + HALF - KNOCK_MS,    /* after "cut" — the big pause */
	QUARTER - KNOCK_MS,           /* after "two"    */
	0,                            /* after "bits" — done */
};

#define NUM_KNOCKS (sizeof(gaps) / sizeof(gaps[0]))

int main(void)
{
	if (!gpio_is_ready_dt(&led)) {
		return 0;
	}

	gpio_pin_configure_dt(&led, GPIO_OUTPUT_INACTIVE);

	/* Small startup delay so the user sees the pattern start cleanly. */
	k_msleep(QUARTER);

	for (int i = 0; i < (int)NUM_KNOCKS; i++) {
		gpio_pin_set_dt(&led, 1);
		k_msleep(KNOCK_MS);
		gpio_pin_set_dt(&led, 0);

		if (gaps[i] > 0) {
			k_msleep(gaps[i]);
		}
	}

	/* Pattern complete — LED stays off until next reset. */
	return 0;
}
