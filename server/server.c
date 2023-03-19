#include <stdio.h>

/* Contiki main files */
#include "contiki.h"
#include "coap-engine.h"

/* Sensors */
#include "dev/adc-zoul.h"
#include "dev/zoul-sensors.h"

/* LWM */
#ifdef COAP_DTLS_KEYSTORE_CONF_WITH_LWM2M
#include "lwm2m-object.h"
#include "lwm2m-engine.h"
#include "lwm2m-security.h"
#endif

/* Log configuration */
#include "sys/log.h"
#define LOG_MODULE "App"
#define LOG_LEVEL LOG_LEVEL_APP
/*
 * Resources to be activated need to be imported through the extern keyword.
 * The build system automatically compiles the resources in the corresponding sub-directory.
 */
extern coap_resource_t res_temp;
  /*res_hello,
  res_leds,
  res_temperature;*/

PROCESS(coap_server, "CoAP server");
AUTOSTART_PROCESSES(&coap_server);

PROCESS_THREAD(coap_server, ev, data)
{
    PROCESS_BEGIN();

#ifdef COAP_DTLS_KEYSTORE_CONF_WITH_LWM2M
    /* Here, we could initialize a keystore with pre-shared keys.
     * We do not use this as of now
     */
    //lwm2m_security_init();
#endif

    /* Activate sensors */
    adc_zoul.configure(SENSORS_HW_INIT, ZOUL_SENSORS_ADC_ALL);

    PROCESS_PAUSE();

    printf("Activating resources\n");

    /* Activate CoAP resources */
    //coap_activate_resource(&res_hello, "hello");
    //coap_activate_resource(&res_leds, "leds/toggle");
    coap_activate_resource(&res_temp, "temperature");
    while (1) {
        PROCESS_WAIT_EVENT();
    }

    printf("Should never happen\n");
    PROCESS_END();
}
