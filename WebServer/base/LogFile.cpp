#include "LogFile.h"
#include <assert.h>
#include <stdio.h>
#include <time.h>
#include "FileUtil.h"


using namespace std;

LogFile::LogFile(const string& basename)
    : basename_(basename),
      mutex_(new MutexLock) {
           file_.reset(new AppendFile(basename));
}

LogFile::~LogFile() {}

void LogFile::append(const char* logline, int len) {
  MutexLockGuard lock(*mutex_);
  append_unlocked(logline, len);
}

void LogFile::flush() {
  MutexLockGuard lock(*mutex_);
  file_->flush();
}

void LogFile::append_unlocked(const char* logline, int len) {
  file_->append(logline, len);
}
