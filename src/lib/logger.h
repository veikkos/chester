#ifndef LOGGER_H
#define LOGGER_H

#include <stdarg.h>
#include <stdint.h>

typedef enum {
  ERROR,
  WARNING,
  INFO,
  DEBUG,
  VERBOSE,
  ALL
} level_e;

typedef level_e level;

#ifndef ERROR_LEVEL
#define ERROR_LEVEL INFO
#endif

#define NEWLINE "\n"

//#define LOG_FILE

#ifndef NDEBUG
void gb_log(const level l, const char *fmt, ...);

void gb_log_stream(const level l, const char c);

void gb_log_close_file();
#else
#define gb_log(l, ...) ;

#define gb_logstream(l, c) ;

#define gb_log_close_file() ;
#endif

void gb_log_print_rom_info(uint8_t *rom);

#endif // LOGGER_H
