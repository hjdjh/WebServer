#pragma once
#include <sys/epoll.h>
#include <unistd.h>
#include <functional>
#include <map>
#include <memory>
#include <string>
#include <unordered_map>
#include "Timer.h"


class EventLoop;
class TimerNode;
class Channel;

enum ProcessState{
	PARSE_URL=1,PARSE_HEADER,PARSE_FINISH
};

enum ResultState{
	PARSE_AGAIN,PARSE_SUCCESS,PARSE_ERROR
};

enum HttpVersion{
	HTTP11,HTTP10
};

enum RequestType{
	GET,HEAD,POST
};

enum ResourceState { RESOURCE_SUCCESS = 1, RESOURCE_ERROR };


enum ConnectionState { H_CONNECTED = 0, H_DISCONNECTING, H_DISCONNECTED };


class MimeType {
 private:
  static void init();
  static std::unordered_map<std::string, std::string> mime;
  MimeType();
  MimeType(const MimeType &m);

 public:
  static std::string getMime(const std::string &suffix);

 private:
  static pthread_once_t once_control;
};

class HttpData : public std::enable_shared_from_this<HttpData> {
 public:
  HttpData(EventLoop *loop, int connfd);
  ~HttpData() { close(fd_); }
  void reset();
  void seperateTimer();
  void linkTimer(std::shared_ptr<TimerNode> mtimer) {
    timer_ = mtimer;
  }
  std::shared_ptr<Channel> getChannel() { return channel_; }
  EventLoop *getLoop() { return loop_; }
  void handleClose();
  void newEvent();

 private:
  EventLoop *loop_;
  std::shared_ptr<Channel> channel_;
  int fd_;
  std::string inBuffer_;
  std::string outBuffer_;
  bool error_;
  ConnectionState connectionState_;

  RequestType requestType_;
  HttpVersion httpVersion_;
  std::string fileName_;
  std::string fileType_;
  std::string path_;
  int nowReadPos_;
  ProcessState processState_;
  ResultState resultState_;
  ResourceState resourceState_;
  bool keepAlive_;
  std::map<std::string, std::string> headers_;
  std::weak_ptr<TimerNode> timer_;

  void handleRead();
  void handleWrite();
  void handleConn();
  void handleError(int fd, int err_num, std::string short_msg);
  void parseURL();
  void parseHeader();
  void handleParseError();
  void analysisRequest();
};
