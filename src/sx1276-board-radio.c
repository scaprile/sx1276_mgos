
#include <stdint.h>
#include <stdbool.h>

#include "sx1276-board.h"

#include "mgos.h"
#include "mgos_gpio.h"
#include "mgos_system.h"
#include "mgos_spi.h"

/* define GPIOs for your hardware
#define SX1276_GPIO_RESET  17 //26
#define SX1276_GPIO_DIO0   22 //36
#define SX1276_GPIO_NSS
*/

/* Here we actually define them in mos.yml
  - ["sx1276.reset_gpio", "i", 26, {title: "GPIO to use for SX1276 reset"}]
  - ["sx1276.dio0_gpio", "i", 36, {title: "GPIO to use for SX1276 DIO0"}]
 * NSS:
This driver uses Mongoose-OS SPI driver and so CS is selected from an array of three options.
Those three options are defined at the SPI configuration structure, e.g.:
  - ["spi.cs0_gpio", "i", 14, {title: "GPIO to use for CS0"}]
  - ["spi.cs1_gpio", "i", 5, {title: "GPIO to use for CS1"}]
  - ["spi.cs2_gpio", "i", -1, {title: "GPIO to use for CS2"}]
The index we choose ourselves in our own config structure. e.g.:
  - ["sx1276.cs_index", "i", 1, {title: "spi.cs*_gpio index, 0, 1 or 2"}]
in order to coexist with other SPI chips like the display (or the TF card) in the M5stack
We've chosen index 1 because the M5stack wrapper uses index 0; you can change them yourself anyway
*/

#define SX1276_SPI_MODE 0           // CPOL = 0, CPHA = 0
#define SX1276_SPI_FREQ 10000000    // (max is 10 MHz)

void SX1276IoInit( void )
{
    mgos_gpio_setup_input(mgos_sys_config_get_sx1276_reset_gpio(), MGOS_GPIO_PULL_UP); // see banner above, SX1276_GPIO_RESET
    mgos_gpio_set_mode(mgos_sys_config_get_sx1276_dio0_gpio(), MGOS_GPIO_MODE_INPUT);  // see banner above, SX1276_GPIO_DIO0
    // SX1276_GPIO_NSS handled by SPI
}

/* Interrupts are invoked in main task context, any function may be used
but service might be delayed. No need to clear trigger */
static void dio_gpio_irq_handler(int pin, void *arg)
{
    DioIrqHandler *handler = (DioIrqHandler *) arg;
    LOG(LL_DEBUG, ("DIO0IRQ, state: %d", SX1276.Settings.State));
    (*handler)(NULL); // function argument is not actually used by the function itself
    (void)pin;
}

void SX1276IoIrqInit( DioIrqHandler **irqHandlers )
{
    int dio0gpio = mgos_sys_config_get_sx1276_dio0_gpio(); // see banner above, SX1276_GPIO_DIO0
    mgos_gpio_set_int_handler(dio0gpio, MGOS_GPIO_INT_EDGE_POS, dio_gpio_irq_handler, irqHandlers[0]);
    mgos_gpio_enable_int(dio0gpio);
    // remaining handlers, if any, have been put in the array in the following order
#ifdef SX1276_BOARD_DIO1
#endif
#ifdef SX1276_BOARD_DIO2
#endif
#ifdef SX1276_BOARD_DIO3
#endif
#ifdef SX1276_BOARD_DIO4
#endif
}

void SX1276Reset( void )
{
    int resetgpio = mgos_sys_config_get_sx1276_reset_gpio(); // see banner above, SX1276_GPIO_RESET
    // Enable the TCXO if available on the board design
    // Set RESET pin to 0
    mgos_gpio_setup_output(resetgpio, 0);
    // Wait 1 ms
    mgos_msleep( 1 );
    // Configure RESET pin as pulled-up
    mgos_gpio_setup_input(resetgpio, MGOS_GPIO_PULL_UP);
    // Wait 6 ms
    mgos_msleep( 6 );

    uint8_t version;
    SX1276ReadBuffer( REG_VERSION, &version, 1);
    LOG(LL_INFO, ("RESET; chip version: 0x%0X", version));
}

void SX1276WriteBuffer( uint32_t addr, uint8_t *buffer, uint8_t len )
{
  // NSS handled by SPI library
  struct mgos_spi *spi = mgos_spi_get_global();
  if (!spi) {
    LOG(LL_ERROR, ("Couldn't get SPI global instance"));
    return;
  }
  struct mgos_spi_txn txn = { 
    .cs   = mgos_sys_config_get_sx1276_cs_index(), // see banner above
    .mode = SX1276_SPI_MODE,
    .freq = SX1276_SPI_FREQ,
  };
  uint8_t temp_buff[256];           // copy address and data to a temporary buffer
  temp_buff[0] = addr | 0x80;       // writes have bit 7 high
  temp_buff[1] = *buffer;
  if (len > 1) {
    memcpy(temp_buff + 2, buffer + 1, len - 1);
  }
 #if 0  // use full-duplex transaction once rx NULL ptr fix is released
  txn.fd.tx_data   = temp_buff;    // then send all data in one single transaction
  txn.fd.len       = len + 1;
  txn.fd.rx_data   = NULL;         // rx is discarded
  mgos_spi_run_txn(spi, true, &txn);    // full-duplex transaction
 #else  // use half-duplex transaction
  txn.hd.tx_data   = temp_buff;    // then send all data in one single transaction
  txn.hd.tx_len    = len + 1;
  txn.hd.rx_data   = NULL;
  txn.hd.rx_len    = 0;
  mgos_spi_run_txn(spi, false, &txn);    // half-duplex transaction
#endif
}

void SX1276ReadBuffer( uint32_t addr, uint8_t *buffer, uint8_t len )
{
  struct mgos_spi *spi = mgos_spi_get_global();
  if (!spi) {
    LOG(LL_ERROR, ("Couldn't get SPI global instance"));
    return;
  }
  struct mgos_spi_txn txn = { 
    .cs   = mgos_sys_config_get_sx1276_cs_index(), // see banner above
    .mode = SX1276_SPI_MODE,
    .freq = SX1276_SPI_FREQ,
  };
  uint8_t reg_addr = (uint8_t)addr & 0x7F;  // reads have bit 7 low
  txn.hd.tx_data    = &reg_addr;           // write address, then read data
  txn.hd.tx_len     = 1;
  txn.hd.dummy_len  = 0;
  txn.hd.rx_data    = buffer;
  txn.hd.rx_len     = len;
  mgos_spi_run_txn(spi, false, &txn);   // half-duplex transaction
}


/* see vendor sx1276-board.c example and decide for your own board */

uint8_t SX1276GetPaSelect( uint32_t channel )
{
//    return RF_PACONFIG_PASELECT_RFO;
    return RF_PACONFIG_PASELECT_PABOOST;
}

void SX1276SetAntSwLowPower( bool status )
{
}

void SX1276SetAntSw( uint8_t opMode )
{
}

void SX1276SetBoardTcxo( uint8_t state )
{
}

uint32_t SX1276GetBoardTcxoWakeupTime( void )
{
	return 0;
}

bool SX1276CheckRfFrequency( uint32_t frequency )
{
    // Implement check. Currently all frequencies are supported
    return true;
}

/*!
 * Radio driver structure initialization
 */
const struct Radio_s Radio =
{
    SX1276Init,
    SX1276GetStatus,
    SX1276SetModem,
    SX1276SetChannel,
    SX1276IsChannelFree,
    SX1276Random,
    SX1276SetRxConfig,
    SX1276SetTxConfig,
    SX1276CheckRfFrequency,
    SX1276GetTimeOnAir,
    SX1276Send,
    SX1276SetSleep,
    SX1276SetStby,
    SX1276SetRx,
    SX1276StartCad,
    SX1276SetTxContinuousWave,
    SX1276ReadRssi,
    SX1276Write,
    SX1276Read,
    SX1276WriteBuffer,
    SX1276ReadBuffer,
    SX1276SetMaxPayloadLength,
    SX1276SetPublicNetwork,
    SX1276GetWakeupTime,
    NULL, // void ( *IrqProcess )( void )
    NULL, // void ( *RxBoosted )( uint32_t timeout ) - SX126x Only
    NULL, // void ( *SetRxDutyCycle )( uint32_t rxTime, uint32_t sleepTime ) - SX126x Only
};
