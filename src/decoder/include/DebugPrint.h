/**
 * @copyright Utilitech AS 2023-2026
 * License: Fair Source
 *
 * Portable Print/Stream interface for the decoder module.
 * On Arduino targets the real Print class is used; on native a minimal
 * implementation is provided so the decoder compiles without Arduino.h.
 */

#ifndef _DEBUGPRINT_H
#define _DEBUGPRINT_H

#if defined(ARDUINO)
  #include <Arduino.h>
#else
  #include <cstdio>
  #include <cstdarg>
  #include <cstring>
  #include <cstdint>

  class Print {
  public:
    virtual ~Print() = default;

    size_t print(const char* s) {
      if (!s) return 0;
      return fputs(s, stdout) >= 0 ? strlen(s) : 0;
    }
    size_t print(uint8_t v, int base = 10) {
      const char* fmt = (base == 16) ? "%X" : "%d";
      return printf(fmt, v);
    }
    size_t println(const char* s = "") {
      return print(s) + print("\n");
    }
    size_t printf(const char* fmt, ...) {
      va_list args;
      va_start(args, fmt);
      int n = vprintf(fmt, args);
      va_end(args);
      return n > 0 ? (size_t)n : 0;
    }
    // printf_P / PSTR are no-ops on non-AVR hardware
    template<typename... Args>
    size_t printf_P(const char* fmt, Args... args) {
      return printf(fmt, args...);
    }
  };

  class Stream : public Print {
  public:
    virtual int available() = 0;
    virtual int read() = 0;
    virtual int peek() = 0;
  };
#endif

#endif
