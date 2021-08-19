#include "Epoll.h"
#include <assert.h>
#include <errno.h>
#include <netinet/in.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <deque>
#include <queue>
#include "Util.h"
#include "base/Logging.h"
#include <arpa/inet.h>
#include <iostream>
using namespace std;
const int EVENTSNUM = 4096;
const int EPOLLWAIT_TIME = 10000;
typedef shared_ptr<Channel> SP_Channel;

Epoll::Epoll() : epollFd_(epoll_create1(EPOLL_CLOEXEC)), events_(EVENTSNUM) {
  assert(epollFd_ > 0);
}
Epoll::~Epoll() {}

void Epoll::epoll_add(SP_Channel request, int timeout) {
  int fd = request->getFd();
  if (timeout > 0) {
    add_timer(request, timeout);
    httpDataManager_[fd] = request->getHolder();
  }
  struct epoll_event event;
  event.data.fd = fd;
  event.events = request->getEvents();
  request->EqualAndUpdateLastEvents();

  channelManager_[fd] = request;
  if (epoll_ctl(epollFd_, EPOLL_CTL_ADD, fd, &event) < 0) {
    perror("epoll_add error");
    channelManager_[fd].reset();
  }
}

void Epoll::epoll_mod(SP_Channel request, int timeout) {
  if (timeout > 0) add_timer(request, timeout);
  int fd = request->getFd();
  if (!request->EqualAndUpdateLastEvents()) {
    struct epoll_event event;
    event.data.fd = fd;
    event.events = request->getEvents();
    if (epoll_ctl(epollFd_, EPOLL_CTL_MOD, fd, &event) < 0) {
      perror("epoll_mod error");
      channelManager_[fd].reset();
    }
  }
}

void Epoll::epoll_del(SP_Channel request) {
  int fd = request->getFd();
  struct epoll_event event;
  event.data.fd = fd;
  event.events = request->getLastEvents();
  if (epoll_ctl(epollFd_, EPOLL_CTL_DEL, fd, &event) < 0) {
    perror("epoll_del error");
  }
  channelManager_[fd].reset();
  httpDataManager_[fd].reset();
}

std::vector<SP_Channel> Epoll::poll() {
  while (true) {
    int event_count =epoll_wait(epollFd_, &*events_.begin(), events_.size(), EPOLLWAIT_TIME);
    if (event_count < 0) perror("epoll wait error");
    std::vector<SP_Channel> req_data = getEventsRequest(event_count);
    if (req_data.size() > 0) return req_data;
  }
}

void Epoll::handleExpired() { timerManager_.handleExpiredEvent(); }

std::vector<SP_Channel> Epoll::getEventsRequest(int events_num) {
  std::vector<SP_Channel> req_data;
  for (int i = 0; i < events_num; ++i) {
    int fd = events_[i].data.fd;
    SP_Channel cur_req = channelManager_[fd];
    if (cur_req) {
      cur_req->setRevents(events_[i].events);
      cur_req->setEvents(0);
      req_data.push_back(cur_req);
    } else {
      LOG << "SP cur_req is invalid";
    }
  }
  return req_data;
}

void Epoll::add_timer(SP_Channel request_data, int timeout) {
  shared_ptr<HttpData> httpData = request_data->getHolder();
  if (httpData)
    timerManager_.addTimer(httpData, timeout);
  else
    LOG << "timer add fail";
}
