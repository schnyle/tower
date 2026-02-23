#ifndef CONFIG_H
#define CONFIG_H

#define CONFIG_DEFAULT_LOGFILE "/var/log/tower/tower.log"
#define CONFIG_DEFAULT_CPU 1
#define CONFIG_DEFAULT_MEM 1
#define CONFIG_DEFAULT_TEMP 1
#define CONFIG_DEFAULT_SHOW_ALL 1

typedef struct
{
  int cpu;
  int mem;
  int temp;
} Metrics;

typedef struct
{
  int show_all;
} TuiConfig;

typedef struct
{
  char logfile[256];
  Metrics metrics;
  TuiConfig tui_config;
} Config;

extern Config config;

int init_config(void);

int read_config(void);

int write_config(void);

#endif
