#ifndef TOWER_LOG_H
#define TOWER_LOG_H

#include <stdarg.h>
#include <stdio.h>
#include <time.h>

#define DEFAULT_LOG_FILE "/var/log/tower.log"

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

static inline void get_timestamp(char *buf, size_t len)
{
  time_t now = time(NULL);
  strftime(buf, len, "%Y-%m-%dT%H:%M:%S%z", localtime(&now));
}

static inline void log_write(LogLevel level, const char *file, int line, const char *fmt, ...)
{
  char timestamp[64];
  get_timestamp(timestamp, sizeof(timestamp));

  // TODO: try to create file if not exists
  FILE *fp = fopen(DEFAULT_LOG_FILE, "a");
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
