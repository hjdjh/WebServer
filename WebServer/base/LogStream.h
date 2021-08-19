#pragma once
#include <assert.h>
#include <string.h>
#include <string>
#include "noncopyable.h"

class AsyncLogging;
const int kSmallBuffer = 4000;
const int kLargeBuffer = 4000 * 1000;

template <int SIZE>
class FixedBuffer : noncopyable {
 public:
  FixedBuffer() : cur_(data_) {}

  ~FixedBuffer() {}

  void append(const char* buf, size_t len) {
    if (avail() > static_cast<int>(len)) {
      memcpy(cur_, buf, len);
      cur_ += len;
    }
  }

  const char* data() const { return data_; }
  int length() const { return static_cast<int>(cur_ - data_); }

  char* current() { return cur_; }
  int avail() const { return static_cast<int>(end() - cur_); }
  void add(size_t len) { cur_ += len; }

  void reset() { cur_ = data_; }
  void bzero() { memset(data_, 0, sizeof data_); }

 private:
  const char* end() const { return data_ + sizeof data_; }

  char data_[SIZE];
  char* cur_;
};

class LogStream : noncopyable {
 public:
  typedef FixedBuffer<kSmallBuffer> Buffer;

  LogStream& operator<<(bool v) {
    buffer_.append(v ? "1" : "0", 1);
    return *this;
  }


  LogStream& operator<<(char v) {
    buffer_.append(&v, 1);
    return *this;
  }

  LogStream& operator<<(const char* str) {
    if (str)
      buffer_.append(str, strlen(str));
    else
      buffer_.append("(null)", 6);
    return *this;
  }

  LogStream& operator<<(const unsigned char* str) {
    return operator<<(reinterpret_cast<const char*>(str));
  }

  LogStream& operator<<(const std::string& v) {
    buffer_.append(v.c_str(), v.size());
    return *this;
  }

  LogStream& operator<<(short v){
	  std::string str=std::to_string(v);
	  buffer_.append(str.c_str(), str.size());
          return *this;
  }

  LogStream& operator<<(unsigned short v){
	  std::string str=std::to_string(v);
          buffer_.append(str.c_str(), str.size());
          return *this;
  }
  LogStream& operator<<(int v){
	  std::string str=std::to_string(v);
          buffer_.append(str.c_str(), str.size());
          return *this;
  }
  LogStream& operator<<(unsigned int v){
	  std::string str=std::to_string(v);
          buffer_.append(str.c_str(), str.size());
          return *this;
  }
  LogStream& operator<<(long v){
	  std::string str=std::to_string(v);
          buffer_.append(str.c_str(), str.size());
          return *this;
  }
  LogStream& operator<<(unsigned long v){
	  std::string str=std::to_string(v);
          buffer_.append(str.c_str(), str.size());
          return *this;
  }
  LogStream& operator<<(long long v){
	  std::string str=std::to_string(v);
          buffer_.append(str.c_str(), str.size());
          return *this;
  }
  LogStream& operator<<(unsigned long long v){
	  std::string str=std::to_string(v);
          buffer_.append(str.c_str(), str.size());
          return *this;
  }
  LogStream& operator<<(const void* v){
  }
  LogStream& operator<<(float v) {
	  std::string str=std::to_string(v);
          buffer_.append(str.c_str(), str.size());
          return *this;
  }
  LogStream& operator<<(double v){
	  std::string str=std::to_string(v);
          buffer_.append(str.c_str(), str.size());
          return *this;
  }
  LogStream& operator<<(long double v){
	  std::string str=std::to_string(v);
          buffer_.append(str.c_str(), str.size());
          return *this;
  }





  void append(const char* data, int len) { buffer_.append(data, len); }
  const Buffer& buffer() const { return buffer_; }
  void resetBuffer() { buffer_.reset(); }

 private:

  Buffer buffer_;

  static const int kMaxNumericSize = 32;
};
