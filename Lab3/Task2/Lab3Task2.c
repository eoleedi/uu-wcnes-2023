#include "contiki.h"
#include "sys/node-id.h"
#include "sys/log.h"
#include "net/ipv6/uip-ds6-route.h"
#include "net/ipv6/uip-sr.h"
#include "net/mac/tsch/tsch.h"
#include "net/routing/routing.h"
#include "dev/leds.h"
#define DEBUG DEBUG_PRINT
#include "net/ipv6/uip-debug.h"

#include "sys/log.h"
     #define LOG_MODULE "App"
     #define LOG_LEVEL LOG_LEVEL_DBG

/*---------------------------------------------------------------------------*/
PROCESS(node_process, "RPL Node");
AUTOSTART_PROCESSES(&node_process);

/*---------------------------------------------------------------------------*/
PROCESS_THREAD(node_process, ev, data)
{
  int is_coordinator;
  int r_count;

  PROCESS_BEGIN();

  is_coordinator = 0;

  is_coordinator = (node_id == 1);

  if(is_coordinator) {
    NETSTACK_ROUTING.root_start();
  }
  NETSTACK_MAC.on();

  {
    static struct etimer et;
    /* Print out routing tables every minute */
    etimer_set(&et, CLOCK_SECOND * 10);
    while(1) {
        LOG_INFO("Routing entries: %u\n", uip_ds6_route_num_routes());
        uip_ds6_route_t *route = uip_ds6_route_head();
        r_count = 0;
      	while(route) {
		LOG_INFO("Route ");
		LOG_INFO_6ADDR(&route->ipaddr);
		LOG_INFO_(" via ");
		LOG_INFO_6ADDR(uip_ds6_route_nexthop(route));
		LOG_INFO_("\n");
		route = uip_ds6_route_next(route);
    r_count++;
	}
  leds_off(LEDS_ALL);
  // assume the root node does not care about being alone
  if(node_id > 1) {
    if(r_count > 0) {
      // we are an intermediate node
      LOG_INFO("Intermediate %d\n", LEDS_BLUE);
      leds_off(LEDS_GREEN | LEDS_RED);
      leds_on(64);
    } else {
      // we are end node
      LOG_INFO("End node %d\n", LEDS_RED);
      leds_off(LEDS_GREEN | 64);
      leds_on(LEDS_RED);
    }
  } else {
    // we are root node
    LOG_INFO("Root node %d\n", LEDS_GREEN);
    leds_off(LEDS_RED | 64);
    leds_on(LEDS_GREEN);
  }
      	PROCESS_YIELD_UNTIL(etimer_expired(&et));
      	etimer_reset(&et);
    }
  }

  PROCESS_END();
}
