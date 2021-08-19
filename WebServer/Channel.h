#pragma once
#include <sys/epoll.h>
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include "Timer.h"

class EventLoop;
class HttpData;

class Channel {
 private:
     typedef std::function<void()> CallBack;
     EventLoop *loop_;
     std::weak_ptr<HttpData> holder_;
     int fd_;

     __uint32_t events_;
     __uint32_t revents_;
     __uint32_t lastEvents_;

     CallBack readHandler_;
     CallBack writeHandler_;
     CallBack connHandler_;

 public:
     Channel(EventLoop *loop);
     Channel(EventLoop *loop, int fd);
     ~Channel();

     int getFd();
     void setFd(int fd);
     void setHolder(std::shared_ptr<HttpData> holder) { holder_ = holder; }

     std::shared_ptr<HttpData> getHolder() {
          std::shared_ptr<HttpData> ret(holder_.lock());
          return ret;
     }

     void setReadHandler(CallBack &&readHandler) { readHandler_ = readHandler; }
     void setWriteHandler(CallBack &&writeHandler) { writeHandler_ = writeHandler;}
     void setConnHandler(CallBack &&connHandler) { connHandler_ = connHandler; }

     void handleEvents();
     void handleRead();
     void handleWrite();
     void handleConn();

     void setRevents(__uint32_t ev) { revents_ = ev; }
     void setEvents(__uint32_t ev) { events_ = ev; }
     __uint32_t &getEvents() { return events_; }
     __uint32_t getLastEvents() { return lastEvents_; }

     bool EqualAndUpdateLastEvents() {
         bool ret = (lastEvents_ == events_);
         lastEvents_ = events_;
         return ret;
     }
};

typedef std::shared_ptr<Channel> SP_Channel;
