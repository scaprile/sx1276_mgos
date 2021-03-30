#  Mongoose-OS implementation of the SX1276/8 driver specifics

- Timers and timing information have been provided by using the services provided by Mongoose-OS.

- Board specifics are based on a simple Ra-01 module.

- GPIOs can be configured using declarations in mos.yml.
  - Default pins are for an M5Stack LoRa868 module (Ra-01H); see mos.yml in this repo
  - Otherwise, change them (again, see mos.yml in this repo. E.g.: for an Espressif ESP32 core board:
``` yaml
    # pins you used in your connection to the SX1276, overwrite library defaults
    - ["sx1276.reset_gpio", 22]
    - ["sx1276.dio0_gpio", 17]
    # choose a CS index and assign it to the pin where you connected NSS
    - ["spi.cs1_gpio", 5]
    - ["sx1276.cs_index", 1]
```

## Example App
The pingpong app provided at the sx1276 repository has not been written for an event-driven environment.

It is just a generic example, an idea on how to use the driver. To be used in such an environment, it should be properly re-written with that in mind. This in turn would involve ensuring the app core gets called after event callbacks are processed, which depends on the framework used and the context in which those events are triggered.

When running it as it is, by performing periodic calls to it, synchronization will be affected since there is some important amount of time between end of transmission and start of reception that the app is not called, and it should.

Too long a periodic call timer, more chance of losing the other end messages. Too short a periodic call timer would impede normal execution of background tasks. Using it as a simple tool to quickly check the hardware and driver are operational (e.g.: not getting TX_TIMEOUTs and being likely to sync), a 10ms timer has been successfully tested

``` C
#include "mgos.h"
#include "mgos_spi.h"

extern void pingpong_init(void);
extern int pingpong(bool *master, int16_t *rssi, int8_t *snr);

static void rundemo(void *userdata) {
  int16_t rssi;
  int8_t snr;
  bool master;

  switch (pingpong(&master, &rssi, &snr)) {
  case 1: // rssi and snr valid for last received message
    if (master) {
      LOG(LL_INFO, ("Got PONG response"));
    } else {
      LOG(LL_INFO, ("Got PING poll"));
    }
    LOG(LL_INFO, ("rssi: %d, snr: %d", rssi, snr));
    break;
  case 2:
    if (master) {
      LOG(LL_INFO, ("Sent PING poll"));
    } else {
      LOG(LL_INFO, ("Sent PONG response"));
    }
    break;
  case 3:
    LOG(LL_ERROR, ("TX TIMEOUT, this should not happen"));
    break;
  }
  (void)userdata;
}

enum mgos_app_init_result mgos_app_init(void) {
  struct mgos_spi *spi = mgos_spi_get_global();

  if (!spi) {
    LOG(LL_ERROR, ("SPI bus missing, set spi.enable=true in mos.yml"));
    return false;
  }

  pingpong_init();
  mgos_set_timer(10, true, rundemo, NULL);
  return MGOS_APP_INIT_SUCCESS;
}
```
In order to remove name clashing, the app is by default omitted from compilation so you have to define the macro SX1276_APP_PINGPONG. In your mos.yml:
``` yaml
cdefs:
  SX1276_APP_PINGPONG:
```

# Disclaimer
The original driver code is copyright Semtech and is provided here as a subtree

The pingpong application example is copyright Semtech and is provided here as part of that subtree


