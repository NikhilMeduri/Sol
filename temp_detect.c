#include "contiki.h"
#include "net/routing/routing.h"
#include "random.h"
#include "net/netstack.h"
#include "net/ipv6/simple-udp.h"

#include <string.h>
#include <stdlib.h>
#include "dev/leds.h"
#include "dev/adc-sensors.h"
#include "cpu/cc2538/soc.h"
#include "cpu/cc2538/dev/sha256.h"
/*---------------------------------------------------------------------------*/
#define ADC_PIN              5
#define SENSOR_READ_INTERVAL (CLOCK_SECOND / 4)
#define LOG_LEVEL LOG_LEVEL_INFO
/*---------------------------------------------------------------------------*/
uint8_t new_hash(sha256_state_t *, const void *, void *);



#include "sys/log.h"
#define LOG_MODULE "App"
#define LOG_LEVEL LOG_LEVEL_INFO

#define WITH_SERVER_REPLY  1
#define UDP_CLIENT_PORT	8765
#define UDP_SERVER_PORT	5678

#define SEND_INTERVAL		  (10 * CLOCK_SECOND)

static struct simple_udp_connection udp_conn;

/*----------*---------------------------------------------------------------------------*/
PROCESS(udp_client_process, "UDP client");
AUTOSTART_PROCESSES(&udp_client_process);
/*---------------------------------------------------------------------------*/
static void
udp_rx_callback(struct simple_udp_connection *c,
         const uip_ipaddr_t *sender_addr,
         uint16_t sender_port,
         const uip_ipaddr_t *receiver_addr,
         uint16_t receiver_port,
         const uint8_t *data,
         uint16_t datalen)
{


  LOG_INFO("Received response '%.*s' from ", datalen, (char *) data);
  LOG_INFO_6ADDR(sender_addr);
#if LLSEC802154_CONF_ENABLED
  LOG_INFO_(" LLSEC LV:%d", uipbuf_get_attr(UIPBUF_ATTR_LLSEC_LEVEL));
#endif
  LOG_INFO_("\n");

}
/*---------------------------------------------------------------------------*/
/*-----------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(udp_client_process, ev, data)
{
  static struct etimer periodic_timer;
  static unsigned count;
  static char str[32];
  uip_ipaddr_t dest_ipaddr;


  sha256_state_t ctx;
  static uint8_t hash[32];
  static uint8_t hash_success;
/* initialize Software on Chip */
  soc_init();
  crypto_enable();
  sha256_init(&ctx);
  uint8_t i,j;
  uint16_t ldr;
  char str_buf[32]="beyer/light/";
  char light_buf[5];


  PROCESS_BEGIN();

  /* Initialize UDP connection */
  simple_udp_register(&udp_conn, UDP_CLIENT_PORT, NULL,
                      UDP_SERVER_PORT, udp_rx_callback);

  etimer_set(&periodic_timer, random_rand() % SEND_INTERVAL);

  /* Use pin number not mask, for example if using the PA5 pin then use 5 */
adc_sensors.configure(ANALOG_GROVE_LIGHT, 5);

  while(1) {
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&periodic_timer));

    ldr = adc_sensors.value(ANALOG_GROVE_LIGHT);

    if(NETSTACK_ROUTING.node_is_reachable() && NETSTACK_ROUTING.get_root_ipaddr(&dest_ipaddr)) {
      /* Send to DAG root */
      LOG_INFO("Sending request %u to ", count);
      LOG_INFO_6ADDR( &dest_ipaddr);
      LOG_INFO_("\n");


  LOG_INFO("light=%d",ldr);
      LOG_INFO_("\n");

      sprintf(light_buf,"%d",ldr);
      strcat(str_buf,light_buf);

      hash_success=new_hash(&ctx,str_buf,hash);
      printf("new_hash=%d\n",hash_success);

      sprintf(light_buf,"%c",'+');
      strcat(str_buf,light_buf);

      for(i=0;i<sizeof(hash);)
      {
       for(j=0; j<4; j++)
       {
        printf("0x%02x ",*(hash+i));
        sprintf(light_buf,"%d",*(hash+i++));
        strcat(str_buf,light_buf);

       }
      printf("\n");
     }




     /* str contains information to send */
      snprintf(str, sizeof(str), "%s %d",str_buf, count); /*bey*/

      simple_udp_sendto(&udp_conn, str, strlen(str), &dest_ipaddr);
      count++;
    } else {
      LOG_INFO("Not reachable yet\n");
    }

    /* Add some jitter */
    etimer_set(&periodic_timer, SEND_INTERVAL
      - CLOCK_SECOND + (random_rand() % (2 * CLOCK_SECOND)));
  }

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
