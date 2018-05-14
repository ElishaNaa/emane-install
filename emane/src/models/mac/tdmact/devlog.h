
#ifndef DEVLOG_HEADER_
#define DEVLOG_HEADER_
// Development log
// It would be ideal to have a logging engine that is
// 1 - does not block threads, because it would slow down the system
// 2 - thread safe
// 3 - different from the standard logger, because it is very clogged
// 4 - writes to a pipe in a nonblocked mode, so it does not clog the hard disk
// 5 - simple, so it has a small footprint
// but the world is hard.., so
// the emane standard log has 1,2,5 (arguably) --- we use it for error messages and things like that
// For development we will use the CTLOG macro defined below. It has 3,4,5. It can be nonblocking

#include <stdarg.h>
#include <string>


// start of .h (if we need to log on more than one file)
//#define DEVLOG_FILE "/home/ubuntu/Python/models/emanedemos/tdmact/TC_log"
#define DEVLOG_USE 1
#define DEVLOG_BLOCKING false



class CTLogger
{
public:
  CTLogger();
  void init(std::string filename, bool blocking);
  bool isInitialized();
  ~CTLogger();
  void log(const char *fmt, ...);

private:
  int writefd_;
  bool blocking_;
  std::string filename_;
  const char *file_;
  bool initialized_;
  void log_i(int fd, const char *fmt, va_list ap);
};

extern CTLogger global_logger;


#if DEVLOG_USE
 #define CTLOG(...)                  \
  if (1)                            \
  {                                 \
    global_logger.log(__VA_ARGS__); \
  }
 #define CTLOG_INIT(filename)        \
 if(!global_logger.isInitialized())                              \
 {                                  \
   global_logger.init(filename, DEVLOG_BLOCKING);    \
 }
#else
 #define CTLOG(...) ;
 #define CTLOG_INIT(filename) ;
#endif


#endif // DEVLOG_HEADER
