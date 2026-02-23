#include "tower/config.h"

#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#define CONFIG_FILE_NAME "tower.conf"
#define CONFIG_PATH_MAX 256

typedef enum
{
  CONFIG_KEY_UNKNOWN,
  CONFIG_LOG_LOGFILE,
  CONFIG_METRICS_CPU,
  CONFIG_METRICS_MEM,
  CONFIG_METRICS_TEMP,
  CONFIG_TUI_SHOW_ALL,
} ConfigKey;

static char config_path[CONFIG_PATH_MAX];

Config config = {
    .logfile = CONFIG_DEFAULT_LOGFILE,
    .metrics =
        {
            .cpu = CONFIG_DEFAULT_CPU,
            .mem = CONFIG_DEFAULT_MEM,
            .temp = CONFIG_DEFAULT_TEMP,
        },
    .tui_config =
        {
            .show_all = CONFIG_DEFAULT_SHOW_ALL,
        },
};

static int get_config_dir(char *buf, size_t len)
{
  const char *xdg_config_home = getenv("XDG_CONFIG_HOME");
  if (xdg_config_home)
  {
    snprintf(buf, len, "%s/tower", xdg_config_home);
    return 0;
  }

  const char *home = getenv("HOME");
  if (home)
  {
    snprintf(buf, len, "%s/.config/tower", home);
    return 0;
  }
  fprintf(stderr, "HOME dir not found\n");

  return -1;
}

int init_config(void)
{
  char path[CONFIG_PATH_MAX];

  char dir[CONFIG_PATH_MAX - 1 - sizeof(CONFIG_FILE_NAME)];
  if (get_config_dir(dir, sizeof(dir)) < 0)
  {
    fprintf(stderr, "error reading config file, using defaults\n");
    return -1;
  };

  snprintf(path, sizeof(path), "%s/%s", dir, CONFIG_FILE_NAME);
  if (mkdir(dir, 0755) < 0 && errno != EEXIST)
  {
    fprintf(stderr, "failed to create config dir %s: %s\n", dir, strerror(errno));
    return -1;
  }

  FILE *fp = fopen(path, "a");
  if (!fp)
  {
    fprintf(stderr, "failed to create config file %s: %s\n", path, strerror(errno));
    return -1;
  }
  fclose(fp);

  strncpy(config_path, path, sizeof(config_path));

  read_config();

  return 0;
}

static char *trim(char *s)
{
  if (*s == '\0')
  {
    return s;
  }

  char *end = s + strlen(s) - 1;
  while (isspace(*s))
  {
    s++;
  }
  while (end > s && isspace(*end))
    end--;
  *(end + 1) = '\0';
  return s;
}

static long parse_long(const char *s, long default_val)
{
  char *end;
  errno = 0;
  long l = strtol(s, &end, 10);
  if (errno || end == s)
  {
    fprintf(stderr, "error converting %s to long, using default\n", s);
    return default_val;
  }
  return l;
}

static ConfigKey parse_key(const char *s)
{
  // LOG
  if (strcmp(s, "logfile") == 0)
    return CONFIG_LOG_LOGFILE;

  // METRICS
  if (strcmp(s, "cpu") == 0)
    return CONFIG_METRICS_CPU;
  if (strcmp(s, "mem") == 0)
    return CONFIG_METRICS_MEM;
  if (strcmp(s, "temp") == 0)
    return CONFIG_METRICS_TEMP;

  // TUI
  if (strcmp(s, "show_all") == 0)
    return CONFIG_TUI_SHOW_ALL;

  return CONFIG_KEY_UNKNOWN;
}

static void parse_config(FILE *fp)
{
  char line[256];

  while (fgets(line, sizeof(line), fp) != NULL)
  {
    int line_len = strcspn(line, "\n");
    line[line_len] = '\0';

    if (line_len == 0)
      continue;

    if (line[0] == '[' && line[line_len - 1] == ']')
      continue;

    char *sep, *val;
    ConfigKey key;
    if ((sep = strchr(line, '=')))
    {
      *sep = '\0';
      key = parse_key(trim(line));
      val = trim(sep + 1);

      switch (key)
      {
      case CONFIG_KEY_UNKNOWN:
        fprintf(stderr, "encountered unknown key: %s\n", line);
        break;
      case CONFIG_LOG_LOGFILE:
        strncpy(config.logfile, val, sizeof(config.logfile) - 1);
        break;
      case CONFIG_METRICS_CPU:
        config.metrics.cpu = parse_long(val, CONFIG_DEFAULT_CPU);
        break;
      case CONFIG_METRICS_MEM:
        config.metrics.mem = parse_long(val, CONFIG_DEFAULT_MEM);
        break;
      case CONFIG_METRICS_TEMP:
        config.metrics.temp = parse_long(val, CONFIG_DEFAULT_TEMP);
        break;
      case CONFIG_TUI_SHOW_ALL:
        config.tui_config.show_all = parse_long(val, CONFIG_DEFAULT_SHOW_ALL);
        break;
      default:
        fprintf(stderr, "unexpected error when parsing config file\n");
      }
    }
  }
}

int read_config(void)
{
  if (config_path[0] == '\0')
  {
    fprintf(stderr, "config file not initialized (was the init function called?)\n");
    return -1;
  }

  FILE *fp = fopen(config_path, "r");
  if (!fp)
  {
    fprintf(stderr, "failed to open config file %s, using defaults: %s\n", config_path, strerror(errno));
    return -1;
  }

  parse_config(fp);
  fclose(fp);

  return 0;
}

int write_config(void)
{
  if (config_path[0] == '\0')
  {
    fprintf(stderr, "config file not initialized (was the init function called?)\n");
    return -1;
  }

  FILE *fp = fopen(config_path, "w");
  if (!fp)
  {
    fprintf(stderr, "failed to write to config file %s: %s\n", config_path, strerror(errno));
    return -1;
  }

  fprintf(fp, "[LOG]\n");
  fprintf(fp, "logfile = %s\n", config.logfile);
  fprintf(fp, "\n");

  fprintf(fp, "[METRICS]\n");
  fprintf(fp, "cpu = %d\n", config.metrics.cpu);
  fprintf(fp, "mem = %d\n", config.metrics.mem);
  fprintf(fp, "temp = %d\n", config.metrics.temp);
  fprintf(fp, "\n");

  fprintf(fp, "[TUI]\n");
  fprintf(fp, "show_all = %d\n", config.tui_config.show_all);

  fclose(fp);

  return 0;
}
