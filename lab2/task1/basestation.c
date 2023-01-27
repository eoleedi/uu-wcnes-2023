#include <stdio.h>
#include <string.h>
#include "contiki.h"
#include "dev/leds.h"
#include "net/netstack.h"
#include "net/nullnet/nullnet.h"

#define LED_INT_ONTIME CLOCK_SECOND * 10
static process_event_t ledOff_event;

/* Declare our "main" process, the basestation_process */
PROCESS(basestation_process, "Clicker basestation");
PROCESS(led_process, "LED handling process");
/* The basestation process should be started automatically when
 * the node has booted. */
AUTOSTART_PROCESSES(&basestation_process, &led_process);

static struct etimer ledETimer;
PROCESS_THREAD(led_process, ev, data)
{
  PROCESS_BEGIN();
  while (1)
  {
    PROCESS_WAIT_EVENT_UNTIL(ev == ledOff_event);
    etimer_set(&ledETimer, LED_INT_ONTIME);
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&ledETimer));
    leds_off(LEDS_ALL);
  }
  PROCESS_END();
}

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
  leds_on(LEDS_GREEN);
  process_post(&led_process, ledOff_event, NULL);
}

/* Our main process. */
PROCESS_THREAD(basestation_process, ev, data)
{
  PROCESS_BEGIN();

  /* Register the event used for lighting up an LED when interrupt strikes. */
  ledOff_event = process_alloc_event();

  /* Initialize NullNet */
  nullnet_set_input_callback(recv);

  PROCESS_END();
}
