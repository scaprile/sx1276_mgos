
#include "mgos.h"
#include "sx1276-board.h"


bool mgos_sx1276_mgos_init(void) {
  SX1276IoInit();
  return true;
}
