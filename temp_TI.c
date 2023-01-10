#include "contiki.h"
#include <stdio.h>
#include "dev/leds.h"
#include "sys/etimer.h"

#if CONTIKI_TARGET_ZOUL
#include "dev/adc-zoul.h"
#include "dev/zoul-sensors.h"
#else
#include "dev/tmp102.h"
#endif

static struct etimer et;

PROCESS(test_onboard_sensors_process, "Test on-board sensors");
AUTOSTART_PROCESSES(&test_onboard_sensors_process);

PROCESS_THREAD(test_onboard_sensors_process, ev, data)
{
  PROCESS_BEGIN();

  static uint16_t temp;


#if CONTIKI_TARGET_ZOUL

#else

  SENSORS_ACTIVATE(tmp102);

#endif

  etimer_set(&et, CLOCK_SECOND);

  while(1) {

    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));


#if CONTIKI_TARGET_ZOUL
    temp = cc2538_temp_sensor.value(CC2538_SENSORS_VALUE_TYPE_CONVERTED);
    printf("Temperature = %d.%u C\n", temp / 1000, temp % 1000);

#else

    temp   = tmp102.value(TMP102_READ);

    printf("Temperature: %d.%u C\n", temp / 100, temp % 100);


#endif

    etimer_reset(&et);
  }

  PROCESS_END();
}
