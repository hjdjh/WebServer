#include"HttpClient.h"
#include<string.h>
#include<string>
#include<vector>
#include"../Util.h"
#include"../EventLoop.h"
#include"../Channel.h"
#include"../base/Logging.h"
#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>
#include <iostream>

using namespace std;
#define CLIENT_EXPIRED_TIME 100
#define DEFAULT_KEEP_ALIVE_TIME 300000

extern string verifyDataAll;
HttpClient::~HttpClient(){

//	close(fd_);
}


HttpClient::HttpClient(EventLoop *loop,string ip,int port) 
     :loop_(loop),serverPort_(port) ,CONNECTED(false),ERROR(false),serverIP_(ip),
	channel_(new Channel(loop_)){
	fd_=socket(PF_INET, SOCK_STREAM, 0);
	channel_->setFd(fd_);
	channel_->setReadHandler(bind(&HttpClient::handleRead, this));
        channel_->setWriteHandler(bind(&HttpClient::handleWrite, this));
        channel_->setConnHandler(bind(&HttpClient::handleConn, this));
}


void HttpClient::connection(){
    struct sockaddr_in servaddr;
    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(serverPort_);
    inet_pton(AF_INET, serverIP_.c_str(), &servaddr.sin_addr);

    cout<<serverIP_<<"   "<<serverPort_<<"\n";
    if (connect(fd_, (struct sockaddr *)&servaddr, sizeof(servaddr)) == 0) {
        setSocketNonBlocking(fd_);
        CONNECTED=true;
	cout<<"connect success\n";
    }
    else {
	    ERROR=true;
	    cout<<"connect errror:"<<fd_<<"\n";
    }
}

void HttpClient::addToPoller(){
	channel_->setEvents(EPOLLIN|EPOLLOUT|EPOLLET);
	if(CONNECTED && !ERROR)loop_->addToPoller(channel_,0);

}

void HttpClient::handleRead(){
	//cout<<"work in handleRead\n";
	if (!ERROR && CONNECTED){
	     bool zero;
	     readn(fd_, inBuffer_, zero);
	}
	//cout<<inBuffer_<<"\n";
	  __uint32_t &events_ = channel_->getEvents();
	  events_ |= EPOLLIN;

}


void HttpClient::handleWrite(){
  if (!ERROR && CONNECTED) {
        __uint32_t &events_ = channel_->getEvents();
        if (writen(fd_, outBuffer_) < 0) {
            ERROR= true;
        }
        if (outBuffer_.size() > 0) events_ |= EPOLLOUT;
//        else if(events_ &EPOLLOUT)events_-=EPOLLOUT;
  }

	
}




void HttpClient::handleConn(){
	//cout<<"work in handleConn\n";
	if(inBuffer_.size()>=verifyDataAll.size()){
		for(int i=0;i<verifyDataAll.size();i++){
		    	if(inBuffer_[i]!=verifyDataAll[i]){
		    	//	int left=i-20>=0? i-20:0;
		    	//	int right=i+20<verifyDataAll.size()? i+20:verifyDataAll.size();
		    		//string err1=inBuffer_.substr(left,right-left+1);
		    		//string err2=verifyDataAll.substr(left,right-left+1);
				//
				//
				LOG<<"\n---------------------------\n"<<inBuffer_<<verifyDataAll.size()<<"\n----------------------------\n";

		    		//cout<<"data error/n"<<err1<<"\n--------\n"<<err2<<"\n";
		    		assert(0);
				}
				
		}
		LOG<<"data true\n";
	        //cout<<"data true\n";
		if(inBuffer_.size()==verifyDataAll.size())inBuffer_.clear();
                 else inBuffer_=inBuffer_.substr(verifyDataAll.size());
                               
	}
         __uint32_t &events_ = channel_->getEvents();
          events_ = events_|EPOLLET|EPOLLIN;
	loop_->updatePoller(channel_,0);
}




