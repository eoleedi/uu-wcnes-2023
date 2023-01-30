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
static process_event_t shaking_event;
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
			process_post(&client_process, shaking_event, NULL);
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

	static char status1[] = "shaking";
	static char status2[] = "clicked";

	PROCESS_BEGIN();

	/* Initialize NullNet */

	nullnet_set_input_callback(recv);

	/* Activate the button sensor. */
	SENSORS_ACTIVATE(button_sensor);

	/* Register the event used for shaking event */
	shaking_event = process_alloc_event();

	/* Loop forever. */
	while (1)
	{
		PROCESS_WAIT_EVENT_UNTIL(ev == shaking_event || (ev == sensors_event &&
														 data == &button_sensor));

		if (ev == shaking_event)
		{
			printf("shaking event triggered");
			nullnet_buf = (uint8_t *)&status1;
			nullnet_len = sizeof(status1);
			memcpy(nullnet_buf, &status1, sizeof(status1));
		}
		else
		{
			printf("clicker event triggered");
			nullnet_buf = (uint8_t *)&status2;
			nullnet_len = sizeof(status2);
			memcpy(nullnet_buf, &status2, sizeof(status2));
		}

		/* Send the content of the packet buffer using the
		 * broadcast handle. */
		NETSTACK_NETWORK.output(NULL);
	}

	PROCESS_END();
}
