
#include <stdio.h>
#include <string.h>
#include "contiki.h"
#include "dev/button-sensor.h"
#include "dev/leds.h"
#include "net/netstack.h"
#include "net/nullnet/nullnet.h"

// What's up with the blue led???
#ifdef LEDS_BLUE
#undef LEDS_BLUE
#define LEDS_BLUE 64
#endif

/*
 * RED led: mote is operational
 * GREEN led: received packet
 * BLUE led: alarm
 *
 * Code assumptions: the nodes are numbered 1..N
 * The number N is small enough to allow us to fit an array of size N+1 in RAM
 */

#define NUM_NODES 5
#define QUEUE_LEN 3
#define TIME_LIMIT (30 * CLOCK_SECOND)
#define LED_INT_ONTIME (5 * CLOCK_SECOND)
#define TRIGGER_LIMIT 3

const int red_led = LEDS_RED;
const int green_led = LEDS_GREEN;
const int blue_led = LEDS_BLUE;

// stupid typesafe C ... this is not what I grew up with!
#define RED_LED ((process_data_t)red_led)
#define GREEN_LED ((process_data_t)green_led)
#define BLUE_LED ((process_data_t)blue_led)

typedef struct eventQ {
  clock_time_t when;
  linkaddr_t who;
  uint8_t id;
} event_t;

//static event_t event_queue[QUEUE_LEN];
//static int write_head = 0;
static process_event_t ledOff_event;

/*---------------------------------------------------------------------------*/
PROCESS(clicker_ng_process, "Clicker NG Process");
PROCESS(led_process, "LED handling process");
AUTOSTART_PROCESSES(&clicker_ng_process, &led_process);

/*---------------------------------------------------------------------------*/

/*
 * src == NULL means the event originated with us
*/
static void add_event(const linkaddr_t *src) {
  int i = 0, valid = 0;//, insert = 0;
  clock_time_t now = clock_time();
  clock_time_t cutoff = now - TIME_LIMIT;
  static clock_time_t trig_time[NUM_NODES + 1];
  int id = (src == NULL) ? 0 : src->u8[0];

  // lol, forgot about underlfow on unsigned integers ...
  if(cutoff >= now)
    cutoff = 1;

  //printf("Nonzero now: %s now>=cutoff: %s\n", now > 0 ? "true" : "false", now >= cutoff ? "true" : "false");
  /* First cycle through all times and retire too old ones */
  for(i = 0; i <= NUM_NODES; i++) {
    if(i == id) {
      trig_time[i] = now;
    }
    if(trig_time[i] < cutoff) {
      trig_time[i] = 0;
    } else {
      //printf("Valid!\n");
      valid++;
    }
  }
  /*event_t *current;
  for(i = 0; i < QUEUE_LEN; i++) {
    current = &event_queue[(i + write_head) % QUEUE_LEN];
    if(current->when == 0) {
      // empty slot, insert here
      current->when = now;
      insert = 1;
      if(src == NULL) {
        current->id = -1;
      } else {
        current->id = src->u8[0];
        memcpy(&(current->who), src, sizeof(linkaddr_t));
      }
      valid++;
      // no need to go through the rest
      break;
    } else {
      // check if this event has expired
      if(current->when + TIME_LIMIT < now) {
        current->when = 0;
        
        if(insert == 0) {
          current->when = now;
          insert = 1;
          if(src == NULL) {
            current->id = -1;
          } else {
            current->id = src->u8[0];
            memcpy(&(current->who), src, sizeof(linkaddr_t));
          }
          valid++;
        }
      } else {
        valid++;
      }
    }
  }*/
  //printf("Valid: %d Limit: %d\n", valid, TRIGGER_LIMIT);

  if(valid >= TRIGGER_LIMIT) {
    printf("ALARM!!!!\n");
    //leds_on(LEDS_BLUE);
    process_post(&led_process, ledOff_event, BLUE_LED);
  }
}

static void recv(const void *data, uint16_t len,
  const linkaddr_t *src, const linkaddr_t *dest) {
  //printf("Received: %s - from %d\n", (char*) data, src->u8[0]);
  //leds_toggle(LEDS_GREEN);
  process_post(&led_process, ledOff_event, GREEN_LED);
  add_event(src);
}

static struct etimer ledETimer;
PROCESS_THREAD(led_process, ev, data)
{
  static int leds = 0;
  static int timer_started = 0;
  static clock_time_t expire[3];
  static clock_time_t now;

  PROCESS_BEGIN();
  while (1)
  {
    PROCESS_WAIT_EVENT_UNTIL(ev == ledOff_event || etimer_expired(&ledETimer));
    now = clock_time();
    if(ev == ledOff_event) {
      // start timer if it has not been done
      if(timer_started == 0) {
        etimer_set(&ledETimer, CLOCK_SECOND);
        timer_started = 1;
      }
      // turn on the given led
      int led_num = (int) data;
      //printf("Led val: %d\n", led_num);
      leds |= led_num;
      if(led_num == LEDS_GREEN) {
        //printf("Turn on GREEN\n");
        expire[0] = now + LED_INT_ONTIME;
      } else if(led_num == LEDS_BLUE) {
        //printf("Turn on BLUE\n");
        expire[1] = now + LED_INT_ONTIME;
      } else if(led_num == LEDS_RED) {
        //printf("Start timer\n");
        expire[2] = now + LED_INT_ONTIME;
      }
    } else {
      // one second has passed, check if any leds should turn off and reset timer
      //printf("Timer fired!");
      //printf(" GREEN off %s BLUE off %s\n", ((now > expire[0]) ? "Yes" : "No"), ((now > expire[1]) ? "Yes" : "No"));
      if(now > expire[0]) {
        leds &= ~LEDS_GREEN;
        leds_off(LEDS_GREEN);
      }
      if(now > expire[1]) {
        leds &= ~LEDS_BLUE;
        leds_off(LEDS_BLUE);
      }
      if(now > expire[2]) {
        leds &= ~LEDS_RED;
        leds_off(LEDS_RED);
      }
      printf("Reset timer\n");
      etimer_set(&ledETimer, CLOCK_SECOND);
    }
    printf("Leds on: %d\n", leds);
    //leds_off(LEDS_GREEN | LEDS_BLUE);
    leds_on(leds);
    /*PROCESS_WAIT_EVENT_UNTIL(ev == ledOff_event);
    etimer_set(&ledETimer, LED_INT_ONTIME);
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&ledETimer));
    leds_off(LEDS_ALL);*/
  }
  PROCESS_END();
}

/*---------------------------------------------------------------------------*/
PROCESS_THREAD(clicker_ng_process, ev, data)
{
  static char payload[] = "hej";

  PROCESS_BEGIN();


  /* Initialize NullNet */
   nullnet_buf = (uint8_t *)&payload;
   nullnet_len = sizeof(payload);
   nullnet_set_input_callback(recv);
  
  /* Activate the button sensor. */
  SENSORS_ACTIVATE(button_sensor);
  //etimer_set(&ledETimer, CLOCK_SECOND);

  while(1) {
  
    PROCESS_WAIT_EVENT_UNTIL(ev == sensors_event &&
			data == &button_sensor);
			
    //leds_toggle(LEDS_RED);
    process_post(&led_process, ledOff_event, RED_LED);
    
    
    memcpy(nullnet_buf, &payload, sizeof(payload));
    nullnet_len = sizeof(payload);

    /* Send the content of the packet buffer using the
     * broadcast handle. */
     NETSTACK_NETWORK.output(NULL);
     /* Fire event to ourselves */
     add_event(NULL);
  }

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
