#include <stdio.h>
#include <string.h>
#include "contiki.h"
#include "dev/leds.h"
#include "net/netstack.h"
#include "net/nullnet/nullnet.h"

/* Declare our "main" process, the basestation_process */
PROCESS(basestation_process, "Clicker basestation");
/* The basestation process should be started automatically when
 * the node has booted. */
AUTOSTART_PROCESSES(&basestation_process);

/* Holds the number of packets received. */
static int count = 0;

/* Callback function for received packets.
 *
 * Whenever this node receives a packet for its broadcast handle,
 * this function will be called.
 *
 * As the client does not need to receive, the function does not do anything
 */
static void recv(const void *data, uint16_t len,
  const linkaddr_t *src, const linkaddr_t *dest) {
    count++;
    /* 0bxxxxx allows us to write binary values */
    /* for example, 0b10 is 2 */
    leds_off(LEDS_ALL);
    leds_on(count & 0b1111);
}

/* Our main process. */
PROCESS_THREAD(basestation_process, ev, data) {
	PROCESS_BEGIN();

	/* Initialize NullNet */
	nullnet_set_input_callback(recv);

	PROCESS_END();
}
