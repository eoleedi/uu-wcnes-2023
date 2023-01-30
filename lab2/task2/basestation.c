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
PROCESS(led1_process, "LED1 handling process");
PROCESS(led2_process, "LED2 handling process");
PROCESS(led3_process, "LED3 handling process");
/* The basestation process should be started automatically when
 * the node has booted. */
AUTOSTART_PROCESSES(&basestation_process, &led1_process, &led2_process, &led3_process);

static struct etimer led1Etimer;
static struct etimer led2Etimer;
static struct etimer led3Etimer;
PROCESS_THREAD(led1_process, ev, data)
{
  PROCESS_BEGIN();
  while (1)
  {
    PROCESS_WAIT_EVENT_UNTIL(ev == ledOff_event);
    etimer_set(&led1Etimer, LED_INT_ONTIME);
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&led1Etimer));
    leds_off(0b0001);
  }
  PROCESS_END();
}
PROCESS_THREAD(led2_process, ev, data)
{
  PROCESS_BEGIN();
  while (1)
  {
    PROCESS_WAIT_EVENT_UNTIL(ev == ledOff_event);
    etimer_set(&led2Etimer, LED_INT_ONTIME);
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&led2Etimer));
    leds_off(0b0010);
  }
  PROCESS_END();
}
// PROCESS_THREAD(led3_process, ev, data)
// {
//   PROCESS_BEGIN();
//   while (1)
//   {
//     PROCESS_WAIT_EVENT_UNTIL(ev == ledOff_event);
//     etimer_set(&led3Etimer, LED_INT_ONTIME);
//     PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&led3Etimer));
//     leds_off(0b0100);
//   }
//   PROCESS_END();
// }
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
  printf("Received: %s\n", (char *)data);
  if (strcmp(data, "shaking"))
  {
    if (!etimer_expired(&led2Etimer))
    {
      leds_on(0b0100);
      process_post(&led3_process, ledOff_event, NULL);
    }

    leds_on(0b0001);
    process_post(&led1_process, ledOff_event, NULL);
  }
  else if (strcmp(data, "clicked"))
  {
    if (!etimer_expired(&led1Etimer))
    {
      leds_on(0b0100);
      process_post(&led3_process, ledOff_event, NULL);
    }

    leds_on(0b0010);
    process_post(&led2_process, ledOff_event, NULL);
  }
  else
  {
    printf("Unknown event");
  }
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
