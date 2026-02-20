#ifndef TOWER_LOG_H
#define TOWER_LOG_H

#include <errno.h>
#include <libgen.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>

#define LOG_PATH_MAX 256

typedef enum
{
  LOG_INFO,
  LOG_DEBUG,
  LOG_WARNING,
  LOG_ERROR,
} LogLevel;

static const char *level_str[] = {
    "INFO",
    "DEBUG",
    "WARNING",
    "ERROR",
};

static char log_path[LOG_PATH_MAX] = {0};

static inline void get_timestamp(char *buf, size_t len)
{
  time_t now = time(NULL);
  strftime(buf, len, "%Y-%m-%dT%H:%M:%S%z", localtime(&now));
}

static inline int get_log_dir(char *buf, size_t len)
{
  const char *xdg_state_home = getenv("XDG_STATE_HOME");
  if (xdg_state_home)
  {
    snprintf(buf, len, "%s/tower", xdg_state_home);
    return 0;
  }

  const char *home = getenv("HOME");
  if (home)
  {
    snprintf(buf, len, "%s/.local/state/tower", home);
    return 0;
  }

  fprintf(stderr, "HOME dir not found\n");
  return -1;
}

static inline int log_init(void)
{
  char dir[LOG_PATH_MAX - 10]; // reserve room for "/tower.log"
  if (get_log_dir(dir, sizeof(dir)) < 0)
  {
    fprintf(stderr, "unable to get log dir, file logging unavailable\n");
    return -1;
  }

  snprintf(log_path, sizeof(log_path), "%s/tower.log", dir);
  if (mkdir(dir, 0755) < 0 && errno != EEXIST)
  {
    fprintf(stderr, "failed to create log dir %s: %s\n", dir, strerror(errno));
    return -1;
  }

  FILE *fp = fopen(log_path, "a");
  if (!fp)
  {
    fprintf(stderr, "failed to create log file %s: %s\n", log_path, strerror(errno));
    return -1;
  }
  fclose(fp);

  return 0;
}

static inline void log_write(LogLevel level, const char *file, int line, const char *fmt, ...)
{
  static int initialized = 0;
  if (!initialized)
  {
    log_init();
    initialized = 1;
  }

  char timestamp[64];
  get_timestamp(timestamp, sizeof(timestamp));

  FILE *fp = fopen(log_path, "a");
  if (fp)
  {
    fprintf(fp, "[%s] [%s] %s:%d ", timestamp, level_str[level], file, line);
    va_list args;
    va_start(args, fmt);
    vfprintf(fp, fmt, args);
    fprintf(fp, "\n");
    va_end(args);
    fclose(fp);
  }

  FILE *out = level >= LOG_WARNING ? stderr : stdout;
  fprintf(out, "[%s] [%s] %s:%d ", timestamp, level_str[level], file, line);
  va_list args;
  va_start(args, fmt);
  vfprintf(out, fmt, args);
  fprintf(out, "\n");
  va_end(args);
}

#define LOG_INFO(fmt, ...) log_write(LOG_INFO, __FILE__, __LINE__, fmt, ##__VA_ARGS__)
#define LOG_WARNING(fmt, ...) log_write(LOG_WARNING, __FILE__, __LINE__, fmt, ##__VA_ARGS__)
#define LOG_ERROR(fmt, ...) log_write(LOG_ERROR, __FILE__, __LINE__, fmt, ##__VA_ARGS__)

#ifdef DEBUG
#define LOG_DEBUG(fmt, ...) log_write(LOG_DEBUG, __FILE__, __LINE__, fmt, ##__VA_ARGS__)
#else
#define LOG_DEBUG(fmt, ...) /* nothing */
#endif

#endif
