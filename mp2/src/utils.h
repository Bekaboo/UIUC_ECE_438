#ifndef __UTILS_H__
#define __UTILS_H__

#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#define log(fd, ...) ({                                      \
    char _time_str[32];                                      \
    struct tm *_timeinfo = localtime(&(time_t){time(NULL)}); \
    strftime(_time_str, 28, "%Y-%m-%d %H:%M:%S", _timeinfo); \
    int _time_len = strlen(_time_str);                       \
    for (int i = 0; i < _time_len; i++) {                    \
      if (_time_str[i] == '\n') {                            \
        _time_str[i] = '\0';                                 \
        break;                                               \
      }                                                      \
    }                                                        \
    struct timeval _timeval;                                 \
    gettimeofday(&_timeval, NULL);                           \
    fprintf(fd, "%s.%06lu: ", _time_str, _timeval.tv_usec);  \
    fprintf(fd, __VA_ARGS__);                                \
})

#define min(a,b) ({                                       \
    typeof(a) _a_temp_;                                   \
    typeof(b) _b_temp_;                                   \
    _a_temp_ = (a);                                       \
    _b_temp_ = (b);                                       \
    _a_temp_ = _a_temp_ < _b_temp_ ? _a_temp_ : _b_temp_; \
})

#define max(a,b) ({                                       \
    typeof(a) _a_temp_;                                   \
    typeof(b) _b_temp_;                                   \
    _a_temp_ = (a);                                       \
    _b_temp_ = (b);                                       \
    _a_temp_ = _a_temp_ < _b_temp_ ? _b_temp_ : _a_temp_; \
})

#endif /* __UTILS_H__ */
