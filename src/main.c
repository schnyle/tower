#include "tower/config.h"
#include "tower/log.h"

int main()
{
  init_config();
  LOG_INFO("configuration and logger initialized");

  return 0;
}
