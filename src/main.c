#include "tower/config.h"
#include "tower/log.h"

int main()
{
  config_init();

  printf("config: %s\n", Config.logfile);

  LOG_INFO("info %s", "string");

  int x = 2437;
  LOG_WARNING("warning %d", x);

  LOG_DEBUG("ahhhhhh");

  LOG_ERROR("goodbye");

  return 0;
}
