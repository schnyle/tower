#include "tower/metric.h"
#include "tower/log.h"
#include <stdio.h>

#define PROC_MEMINFO_PATH "/proc/meminfo"

void get_proc_meminfo(MemInfo *info)
{
  memset(info, 0xFF, sizeof(*info));

  char line[256];
  char key[64];
  long value;

  FILE *fp = fopen(PROC_MEMINFO_PATH, "r");
  if (!fp)
  {
    LOG_ERROR("failed to open %s", PROC_MEMINFO_PATH);
    return;
  }

  while (fgets(line, sizeof(line), fp) != NULL)
  {
    if (sscanf(line, "%63s %ld", key, &value) != 2)
      continue;

    if (strcmp(key, "MemTotal:") == 0)
      info->total_kb = value;

    if (strcmp(key, "MemFree:") == 0)
      info->free_kb = value;

    if (strcmp(key, "MemAvailable:") == 0)
      info->available_kb = value;

    if (strcmp(key, "SwapTotal:") == 0)
      info->swap_total_kb = value;

    if (strcmp(key, "SwapFree:") == 0)
      info->swap_free_kb = value;
  }

  fclose(fp);
}
