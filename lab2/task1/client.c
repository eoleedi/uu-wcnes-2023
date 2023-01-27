#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "contiki.h"
#include "dev/button-sensor.h"
#include "dev/leds.h"
#include "net/netstack.h"
#include "net/nullnet/nullnet.h"
#include "dev/adxl345.h"

#define ERROR_x 100
#define ACCM_READ_INTERVAL CLOCK_SECOND
/* Declare our "main" process, the client process*/
PROCESS(intrusion_detection_process, "Intrusion detection");
PROCESS(client_process, " Client");
/* The client process should be started automatically when
 * the node has booted. */
AUTOSTART_PROCESSES(&client_process, &intrusion_detection_process);

/* Callback function for received packets.
 *
 * Whenever this node receives a packet for its broadcast handle,
 * this function will be called.
 *
 * As the client does not need to receive, the function does not do anything
 */
static void recv(const void *data, uint16_t len,
				 const linkaddr_t *src, const linkaddr_t *dest)
{
}

PROCESS_THREAD(intrusion_detection_process, ev, data)
{
	static struct etimer et;
	PROCESS_BEGIN();
	static int x = 0;
	static int next_x = 0;

	/* Start and setup the accelerometer with default values, eg no interrupts enabled. */
	accm_init();

	x = accm_read_axis(X_AXIS);
	while (1)
	{
		next_x = accm_read_axis(X_AXIS);
		printf("%d -> %d\n", x, next_x);
		if (x != next_x && abs(next_x - x) > ERROR_x)
		{
			printf("Shaking! Reach the threshold\n");
			process_poll(&client_process);
		}
		x = next_x;
		etimer_set(&et, ACCM_READ_INTERVAL);
		PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));
	}
	PROCESS_END();
}

/* Our main process. */

PROCESS_THREAD(client_process, ev, data)
{

	static char payload[] = "hej";

	PROCESS_BEGIN();

	/* Initialize NullNet */
	nullnet_buf = (uint8_t *)&payload;
	nullnet_len = sizeof(payload);
	nullnet_set_input_callback(recv);

	/* Loop forever. */
	while (1)
	{
		PROCESS_WAIT_EVENT_UNTIL(ev == PROCESS_EVENT_POLL);
		/* Copy the string "hej" into the packet buffer. */
		memcpy(nullnet_buf, &payload, sizeof(payload));
		nullnet_len = sizeof(payload);
		/* Send the content of the packet buffer using the
		 * broadcast handle. */
		NETSTACK_NETWORK.output(NULL);
	}

	PROCESS_END();
}
