/**
 * \A part of the temperature recording module, using the DHT22 sensor
 * @{
 *
 * \defgroup zoul-dht22-test DHT22 temperature and humidity sensor test
 *
 * Demonstrates the use of the DHT22 digital temperature and humidity sensor
 * 
 */
/*---------------------------------------------------------------------------*/
#include "contiki.h"
#include <stdio.h>
#include "dev/leds.h"
#include "sys/etimer.h"
#include "dev/dht22.h"
/*---------------------------------------------------------------------------*/
#define DEBUG 0
#if DEBUG
#define PRINTF(...) printf(__VA_ARGS__)
#else
#define PRINTF(...)
#endif

/*---------------------------------------------------------------------------*/
#define DHT22_PORT_BASE          GPIO_PORT_TO_BASE(DHT22_PORT)
#define DHT22_PIN_MASK           GPIO_PIN_MASK(DHT22_PIN)
/*---------------------------------------------------------------------------*/
static uint8_t enabled;
static uint8_t busy;
static uint8_t dht22_data[DHT22_BUFFER];

PROCESS(remote_dht22_process, "DHT22 test");
AUTOSTART_PROCESSES(&remote_dht22_process);
/*---------------------------------------------------------------------------*/
static struct etimer et;

/*---------------------------------------------------------------------------*/
PROCESS_THREAD(remote_dht22_process, ev, data)
{
  int16_t temperature, humidity;

  PROCESS_BEGIN();
  SENSORS_ACTIVATE(dht22);

  /* Let it spin and read sensor data */

  while(1) {
    etimer_set(&et, CLOCK_SECOND);
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));

    /* The standard sensor API may be used to read sensors individually, using
     * the `dht22.value(DHT22_READ_TEMP)` and `dht22.value(DHT22_READ_HUM)`,
     * however a single read operation (5ms) returns both values, so by using
     * the function below we save one extra operation
     */
    if(dht22_read_all(&temperature, &humidity) != DHT22_ERROR) {
      printf("Temperature %02d.%02d ºC, ", temperature / 10, temperature % 10);
      printf("Humidity %02d.%02d RH\n", humidity / 10, humidity % 10);
    } else {
      printf("Failed to read the sensor\n");
    }
  }
  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
/**
 * @}
 * @}
 */

static int
dht22_read(void)
{
  uint8_t i;
  uint8_t j = 0;
  uint8_t last_state = 0xFF;
  uint8_t counter = 0;
  uint8_t checksum = 0;

  if(enabled) {
    /* Exit low power mode and initialize variables */
    GPIO_SET_OUTPUT(DHT22_PORT_BASE, DHT22_PIN_MASK);
    GPIO_SET_PIN(DHT22_PORT_BASE, DHT22_PIN_MASK);
    RTIMER_BUSYWAIT(DHT22_AWAKE_TIME);
    memset(dht22_data, 0, DHT22_BUFFER);

    /* Initialization sequence */
    GPIO_CLR_PIN(DHT22_PORT_BASE, DHT22_PIN_MASK);
    RTIMER_BUSYWAIT(DHT22_START_TIME);
    GPIO_SET_PIN(DHT22_PORT_BASE, DHT22_PIN_MASK);
    clock_delay_usec(DHT22_READY_TIME);

    /* Prepare to read, DHT22 should keep line low 80us, then 80us high.
     * The ready-to-send-bit condition is the line kept low for 50us, then if
     * the line is high between 24-25us the bit sent will be "0" (zero), else
     * if the line is high between 70-74us the bit sent will be "1" (one).
     */
    GPIO_SET_INPUT(DHT22_PORT_BASE, DHT22_PIN_MASK);

    for(i = 0; i < DHT22_MAX_TIMMING; i++) {
      counter = 0;
      while(GPIO_READ_PIN(DHT22_PORT_BASE, DHT22_PIN_MASK) == last_state) {
        counter++;
        clock_delay_usec(DHT22_READING_DELAY);

        /* Exit if not responsive */
        if(counter == 0xFF) {
          break;
        }
      }

      last_state = GPIO_READ_PIN(DHT22_PORT_BASE, DHT22_PIN_MASK);

      /* Double check for stray sensor */
      if(counter == 0xFF) {
        break;
      }

      /* Ignore the first 3 transitions (the 80us x 2 start condition plus the
       * first ready-to-send-bit state), and discard ready-to-send-bit counts
       */
      if((i >= 4) && ((i % 2) == 0)) {
        dht22_data[j / 8] <<= 1;
        if(counter > DHT22_COUNT) {
          dht22_data[j / 8] |= 1;
        }
        j++;
      }
    }

    for(i = 0; i < DHT22_BUFFER; i++) {
      PRINTF("DHT22: (%u) %u\n", i, dht22_data[i]);
    }

    /* If we have 5 bytes (40 bits), wrap-up and end */
    if(j >= 40) {
      /* The first 2 bytes are humidity values, the next 2 are temperature, the
       * final byte is the checksum
       */
      checksum = dht22_data[0] + dht22_data[1] + dht22_data[2] + dht22_data[3];
      checksum &= 0xFF;
      if(dht22_data[4] == checksum) {
        GPIO_SET_INPUT(DHT22_PORT_BASE, DHT22_PIN_MASK);
        GPIO_SET_PIN(DHT22_PORT_BASE, DHT22_PIN_MASK);
        return DHT22_SUCCESS;
      }
      PRINTF("DHT22: bad checksum\n");
    }
  }
  return DHT22_ERROR;
}
/*---------------------------------------------------------------------------*/
static uint16_t
dht22_humidity(void)
{
  uint16_t res;
  res = dht22_data[0];
  res *= 256;
  res += dht22_data[1];
  busy = 0;
  return res;
}
/*---------------------------------------------------------------------------*/
static uint16_t
dht22_temperature(void)
{
  uint16_t res;
  res = dht22_data[2] & 0x7F;
  res *= 256;
  res += dht22_data[3];
  busy = 0;
  return res;
}
/*---------------------------------------------------------------------------*/
static int
value(int type)
{
  if((type != DHT22_READ_HUM) && (type != DHT22_READ_TEMP) &&
     (type != DHT22_READ_ALL)) {
    PRINTF("DHT22: Invalid type %u\n", type);
    return DHT22_ERROR;
  }

  if(busy) {
    PRINTF("DHT22: ongoing operation, wait\n");
    return DHT22_BUSY;
  }

  busy = 1;

  if(dht22_read() != DHT22_SUCCESS) {
    PRINTF("DHT22: Fail to read sensor\n");
    GPIO_SET_INPUT(DHT22_PORT_BASE, DHT22_PIN_MASK);
    GPIO_SET_PIN(DHT22_PORT_BASE, DHT22_PIN_MASK);
    busy = 0;
    return DHT22_ERROR;
  }

  switch(type) {
  case DHT22_READ_HUM:
    return dht22_humidity();
  case DHT22_READ_TEMP:
    return dht22_temperature();
  case DHT22_READ_ALL:
    return DHT22_SUCCESS;
  default:
    return DHT22_ERROR;
  }
}
/*---------------------------------------------------------------------------*/
int16_t
dht22_read_all(int16_t *temperature, int16_t *humidity)
{
  if((temperature == NULL) || (humidity == NULL)) {
    PRINTF("DHT22: Invalid arguments\n");
    return DHT22_ERROR;
  }

  if(value(DHT22_READ_ALL) != DHT22_ERROR) {
    *temperature = dht22_temperature();
    *humidity = dht22_humidity();
    return DHT22_SUCCESS;
  }

  /* Already cleaned-up in the value() function */
  return DHT22_ERROR;
}
