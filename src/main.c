#include "tower/log.h"

int main()
{
  LOG_INFO("info %s", "string");

  int x = 2437;
  LOG_WARNING("warning %d", x);

  LOG_DEBUG("ahhhhhh");

  LOG_ERROR("goodbye");

  return 0;
}
