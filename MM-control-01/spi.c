#include "spi.h"

extern inline void spi_init();
extern inline void spi_setup(uint8_t spcr, uint8_t spsr);
extern inline uint8_t spi_txrx(uint8_t tx);
