#ifndef CONFIG_H
#define CONFIG_H

#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#define CONFIG_PATH_MAX 256

static struct
{
  char *logfile;
} Config;

static char config_path[CONFIG_PATH_MAX];

static inline void get_config_dir(char *buf, size_t len)
{
  const char *xdg_config_home = getenv("XDG_CONFIG_HOME");
  if (xdg_config_home)
  {
    snprintf(buf, len, "%s/tower", xdg_config_home);
    return;
  }

  const char *home = getenv("HOME");
  if (home)
  {
    snprintf(buf, len, "%s/.config/tower", home);
    return;
  }
  fprintf(stderr, "HOME dir not found\n");

  snprintf(buf, len, "/etc/tower");

  return;
}

static inline char *trim(char *s)
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

static inline void parse_config(FILE *fp)
{
  char line[256];
  char section[256];

  while (fgets(line, sizeof(line), fp) != NULL)
  {
    int line_len = strcspn(line, "\n");
    line[line_len] = '\0';

    if (line_len == 0)
    {
      continue;
    }

    if (line[0] == '[' && line[line_len - 1] == ']')
    {
      strncpy(section, line + 1, line_len);
      section[line_len - 2] = '\0';
      continue;
    }

    char *sep, *key, *val;
    if ((sep = strchr(line, '=')))
    {
      *sep = '\0';
      key = trim(line);
      val = trim(sep + 1);

      if (strncmp(section, "LOG", 3) == 0)
      {
        if (strncmp(key, "logfile", 7) == 0)
        {
          Config.logfile = strdup(val);
        }
      }
    }
  }
}

static inline int config_init(void)
{
  char dir[CONFIG_PATH_MAX - 11]; // reserve room for "/tower.conf"
  get_config_dir(dir, sizeof(dir));

  snprintf(config_path, sizeof(config_path), "%s/tower.conf", dir);
  if (mkdir(dir, 0755) < 0 && errno != EEXIST)
  {
    fprintf(stderr, "failed to create config dir %s: %s\n", dir, strerror(errno));
    return -1;
  }

  FILE *fp = fopen(config_path, "r");
  if (!fp)
  {
    fprintf(stderr, "failed to create config file %s: %s\n", config_path, strerror(errno));
    return -1;
  }

  parse_config(fp);

  fclose(fp);

  return 0;
}

#endif
