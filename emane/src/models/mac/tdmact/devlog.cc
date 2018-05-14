#include "devlog.h"
#include "emane/types.h"

// NB
// Writes to a named pipe DEVLOG_FILE that should be left open (otherwise  it blocks)
#include <fstream>
//#include <sstream> // for ostringstream
#include <string>
#include <mutex>
//#include "date.h"

#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>
#include <unistd.h>


void round(EMANE::TimePoint &time);

CTLogger global_logger;

CTLogger::~CTLogger()
{
  close(writefd_);
}

CTLogger::CTLogger()
{
  initialized_ = false;
}

std::mutex logInit;

void CTLogger::init(std::string filename, bool blocking)
{ 
  logInit.lock();
  initialized_ = true;
  filename_ = filename;
  file_ = filename_.c_str();
  printf("Log in %s\n", file_);
  blocking_ = blocking;
  if (blocking)
  {
    return; // ugly. The blocking logging opens and closes the pipe each time so nothing to do here
  }
  signal(SIGPIPE, SIG_IGN); // UGLY CODE. At initialization it blocks SIGPIPE signals because we need to write to a closable pipe
  struct stat status;
  
  if (-1 == mkfifo(file_, S_IRWXU | S_IRWXG | S_IRWXO))
  {
    if (!(stat(file_, &status) < 0)) // if it exists, do not print error
    {
      fprintf(stderr, "Error in creating fifo\n");
    }
  }
  int readfd = open(file_, O_RDONLY | O_NONBLOCK); // need to open the pipe for reading (instead won't open it for writing in nonblocking mode )
  if (-1 == readfd)
  {
    perror("CTLogger:readfd:open()");
    //exit(EXIT_FAILURE);
  }
  if (-1 == fstat(readfd, &status))
  {
    perror("CTLogger:readfd:fstat()");
    close(readfd);
    //exit(EXIT_FAILURE);
  }
  if (!S_ISFIFO(status.st_mode))
  {
    printf("CTLogger: %s in not a fifo!\n", "myfifo");
    close(readfd);
    //exit(EXIT_FAILURE);
  }

  writefd_ = open(file_, O_WRONLY | O_NONBLOCK);
  if (writefd_ == -1)
  {
    perror("CTLogger: error in open");
  }
  logInit.unlock();
}

void CTLogger::log(const char *fmt, ...)
{
  va_list ap;
  va_start(ap, fmt);
  if (blocking_)
  {
    int temp_fd = open(file_, O_WRONLY);
    log_i(temp_fd, fmt, ap);
    close(temp_fd);
  }
  else
  {
    log_i(writefd_, fmt, ap);
  }
  va_end(ap);
}

bool CTLogger::isInitialized()
{
  return initialized_;
}

void CTLogger::log_i(int fd, const char *fmt, va_list ap)
{
  if (fd == -1)
    return; // just return
  auto now = EMANE::Clock::now();
  std::time_t t{EMANE::Clock::to_time_t(now)};

  std::tm ltm;
  localtime_r(&t, &ltm);

  const int iBuffSize{10000 + 22};
  char buff[iBuffSize] = {0};

  int iLen = snprintf(buff, sizeof(buff), "%02d:%02d:%02d.%06lu ",
                      ltm.tm_hour,
                      ltm.tm_min,
                      ltm.tm_sec,
                      std::chrono::duration_cast<EMANE::Microseconds>(now.time_since_epoch()).count() % 1000000);

  int len = vsnprintf(buff + iLen, iBuffSize - iLen, fmt, ap);
  int bytes = write(fd, buff, len + iLen);
  if (-1 == bytes)
  {
    // ignore
  }
}

