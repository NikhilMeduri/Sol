/* Standard library */
#include <string.h>
#include <stdlib.h>

/* Sensors */
#include "dev/adc-zoul.h"
#include "dev/zoul-sensors.h"

#include "dev/leds.h"

/* CoAP engine */
#include "coap-engine.h"

/* A counter to keep track of the number of sent messages */
static int counter = 0;

static void res_get_handler(coap_message_t *request, coap_message_t *response, uint8_t *buffer, uint16_t preferred_size, int32_t *offset);

/* Define the resource */
RESOURCE(res_temp,
         "title=\"Temperature\";rt=\"JSON\"",
         res_get_handler,
         NULL,
         NULL,
         NULL);

static void
res_get_handler(coap_message_t *request, coap_message_t *response, uint8_t *buffer, uint16_t preferred_size, int32_t *offset)
{
    /* Receive sensor values and encode them */
    char message[COAP_MAX_CHUNK_SIZE] = "";
    int16_t temperature = cc2538_temp_sensor.value(CC2538_SENSORS_VALUE_TYPE_CONVERTED);
    char alert[500];
      if(temperature > 27000) {
        //leds_toggle(LEDS_RED);
         char msg[]  = "Heating Levels Exceeding Normal";
         for(int i=0; i<sizeof(msg);i++)
             alert[i] = msg[i];
      } else if (temperature == 22000 ){
        leds_toggle(LEDS_BLUE);
        char msg[] =  "Check Temperature Levels";
        for(int i=0; i<sizeof(msg);i++)
             alert[i] = msg[i];
      } else if (temperature >= 19000 && temperature <= 21999) {
        leds_toggle(LEDS_GREEN);
        char msg[] =  "Normal Temperature Levels";
        for(int i=0; i<sizeof(msg);i++)
             alert[i] = msg[i];
      }else {
        leds_toggle(LEDS_GREEN);
        char msg[] = "Atmospheric Temperature Levels";
        for(int i=0; i<sizeof(msg);i++)
             alert[i] = msg[i];
      }

  //int result = snprintf(message, COAP_MAX_CHUNK_SIZE - 1,"{\"msg_id\": %d, \"temperature\": %d.%02d, \"Alert\": %s, \"LED\": %d}", COAP_MAX_CHUNK_SIZE, (temperature/1000), (temperature%1000), alert, leds_get());

  int result = snprintf(message, COAP_MAX_CHUNK_SIZE - 1,"{\"Room\": Living, \"temp\": %d.%02d, \"Alert\": %s}", (temperature/1000), (temperature%1000), alert);

    counter++;
    /* Send messages if encoding succeeded */
    if (result < 0) {
        puts("Error while encoding message");
    } else {
        puts("Sending temperature value");

        int length = strlen(message);
        memcpy(buffer, message, length);

        coap_set_header_content_format(response, APPLICATION_JSON);
        coap_set_header_etag(response, (uint8_t*)&length, 1);
        coap_set_payload(response, buffer, length);
    }
}
