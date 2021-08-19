#include "HttpData.h"
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <iostream>
#include "Channel.h"
#include "EventLoop.h"
#include "Util.h"
#include "time.h"

using namespace std;

pthread_once_t MimeType::once_control = PTHREAD_ONCE_INIT;
std::unordered_map<std::string, std::string> MimeType::mime;

const __uint32_t DEFAULT_EVENT = EPOLLIN | EPOLLET | EPOLLONESHOT;
const int DEFAULT_EXPIRED_TIME = 2000;              // ms
const int DEFAULT_KEEP_ALIVE_TIME = 5 * 60 * 1000;  // ms


void MimeType::init() {
  mime[".html"] = "text/html";
  mime[".avi"] = "video/x-msvideo";
  mime[".bmp"] = "image/bmp";
  mime[".c"] = "text/plain";
  mime[".doc"] = "application/msword";
  mime[".gif"] = "image/gif";
  mime[".gz"] = "application/x-gzip";
  mime[".htm"] = "text/html";
  mime[".ico"] = "image/x-icon";
  mime[".jpg"] = "image/jpeg";
  mime[".png"] = "image/png";
  mime[".txt"] = "text/plain";
  mime[".mp3"] = "audio/mp3";
  mime["default"] = "text/html";
}

std::string MimeType::getMime(const std::string &suffix) {
  pthread_once(&once_control, MimeType::init);
  if (mime.find(suffix) == mime.end())
    return mime["default"];
  else
    return mime[suffix];
}

HttpData::HttpData(EventLoop *loop, int connfd)
    : loop_(loop),
      channel_(new Channel(loop, connfd)),
      fd_(connfd),
      error_(false),
      connectionState_(H_CONNECTED),
      requestType_(GET),
      httpVersion_(HTTP11),
      nowReadPos_(0),
      processState_(PARSE_URL),
      resultState_(PARSE_AGAIN),
      keepAlive_(false) {
  channel_->setReadHandler(bind(&HttpData::handleRead, this));
  channel_->setWriteHandler(bind(&HttpData::handleWrite, this));
  channel_->setConnHandler(bind(&HttpData::handleConn, this));
}

void HttpData::reset() {
  fileName_.clear();
  nowReadPos_ = 0;
  processState_=PARSE_URL;
  resultState_=PARSE_AGAIN;
  headers_.clear();
  if (timer_.lock()) {
    shared_ptr<TimerNode> my_timer(timer_.lock());
    my_timer->clearReq();
    timer_.reset();
  }
}

void HttpData::seperateTimer() {
  if (timer_.lock()) {
    shared_ptr<TimerNode> my_timer(timer_.lock());
    my_timer->clearReq();
    timer_.reset();
  }
}

void HttpData::handleClose() {
  connectionState_ = H_DISCONNECTED;
  shared_ptr<HttpData> guard(shared_from_this());
  loop_->removeFromPoller(channel_);
}

void HttpData::newEvent() {
  channel_->setEvents(DEFAULT_EVENT);
  loop_->addToPoller(channel_, DEFAULT_EXPIRED_TIME);
  std::cout<<"work in HttpData newEvent\n";
}
                      

void HttpData::handleRead(){
   std::cout<<"work in handleRead\n";
    bool zero=false;
    int read_num = readn(fd_, inBuffer_, zero);
    LOG << "Request: " << inBuffer_;
    if (connectionState_ == H_DISCONNECTING) {
      inBuffer_.clear();
      return;
    }
    if (read_num < 0) {
      perror("1");
      error_ = true;
      handleError(fd_, 400, "Bad Request");
      return;
    }
    if (zero)connectionState_ = H_DISCONNECTING;
      if (read_num == 0) return;

	LOG<<inBuffer_.substr(0,nowReadPos_);
	while(true){
    if(processState_==PARSE_URL)parseURL();
    else if(processState_==PARSE_HEADER)parseHeader();
    if(processState_==PARSE_FINISH){
		this->analysisRequest();
		cout<<inBuffer_.size()<<"----------------\n";
		 this->reset();

    }
    if(inBuffer_.find("\r\n",nowReadPos_)==string::npos)break;
	}
    handleWrite();
    __uint32_t &events_ = channel_->getEvents();
     if (outBuffer_.size() > 0) events_ |= EPOLLOUT;
     cout<<"outBuffer size :"<<outBuffer_.size()<<endl;

}

void HttpData::handleWrite() {
	std::cout<<"work in handleWrite " <<outBuffer_.size()<<"\n";

  if (!error_ && connectionState_ != H_DISCONNECTED) {
    __uint32_t &events_ = channel_->getEvents();
    if (writen(fd_, outBuffer_) < 0) {
      perror("writen");
      events_ = 0;
      error_ = true;
    }
    if (outBuffer_.size() > 0) events_ |= EPOLLOUT;
  }
}

void HttpData::handleConn() {
	std::cout<<"work in handleConn\n";
  seperateTimer();
  __uint32_t &events_ = channel_->getEvents();
  if (!error_ && connectionState_ == H_CONNECTED) {
    if (events_ != 0) {
      int timeout = DEFAULT_EXPIRED_TIME;
      if (keepAlive_) timeout = DEFAULT_KEEP_ALIVE_TIME;
      if ((events_ & EPOLLIN) && (events_ & EPOLLOUT)) {
        events_ = __uint32_t(0);
        events_ |= EPOLLOUT;
      }
      events_ |= EPOLLET;
      loop_->updatePoller(channel_, timeout);

    } else if (keepAlive_) {
      events_ |= (EPOLLIN | EPOLLET);
      int timeout = DEFAULT_KEEP_ALIVE_TIME;
      loop_->updatePoller(channel_, timeout);
    } else {
      events_ |= (EPOLLIN | EPOLLET);
      int timeout = (DEFAULT_KEEP_ALIVE_TIME >> 1);
      loop_->updatePoller(channel_, timeout);
    }
  } else if (!error_ && connectionState_ == H_DISCONNECTING &&
             (events_ & EPOLLOUT)) {
    events_ = (EPOLLOUT | EPOLLET);
  } else {
    loop_->runInLoop(bind(&HttpData::handleClose, shared_from_this()));
  }
}


void HttpData::handleParseError(){
	cout<<"parse error\n";
	nowReadPos_=0;
	inBuffer_.clear();
	processState_=PARSE_URL;
	resultState_=PARSE_AGAIN;
}

void HttpData::parseURL() {
	if(resultState_==PARSE_AGAIN){
		int pos=inBuffer_.find("\r\n");
		if(pos==string::npos)return;
		do{
			string url=inBuffer_.substr(0,pos);

			if(url.find("GET")!=string::npos)requestType_=GET;
			else if(url.find("HEAD")!=string::npos)requestType_=HEAD;
			      else if(url.find("POST")!=string::npos)requestType_=POST;
			        else{
			        	resultState_=PARSE_ERROR;
			        	break;
					}

			int left=url.find('/');
			int right=url.find(' ',left);
			if(left==string::npos ||right==string::npos){
				resultState_=PARSE_ERROR;
				break;
			}
			else if(right-left==1)fileName_+="index.html";
			     else fileName_=url.substr(left+1,right-left-1);

			if(url.find("HTTP/1.1")!=string::npos)httpVersion_=HTTP11;
			else if(url.find("HTTP/1.0")!=string::npos)httpVersion_=HTTP10;
			     else{
			     	resultState_=PARSE_ERROR;
			        	break;
				 }
			resultState_=PARSE_SUCCESS;

		}while(false);

	    if(resultState_==PARSE_ERROR){
	    	handleParseError();

		}
		else if(resultState_==PARSE_SUCCESS){
	//	        cout<<"parseURL success "<<pos<<endl;;
			nowReadPos_=pos+2;
			processState_=PARSE_HEADER;
			resultState_=PARSE_AGAIN;
		}
	}
}

void HttpData::parseHeader() {
	//cout<<"work in parse header\n";
	if(resultState_==PARSE_AGAIN){
		if(inBuffer_.size()<=nowReadPos_)return;
		int pos_end=inBuffer_.find("\r\n",nowReadPos_);
		if(pos_end==string::npos)return;
		string header_line=inBuffer_.substr(nowReadPos_,pos_end-nowReadPos_);
        do{
		   if(pos_end==nowReadPos_){
		   	  resultState_=PARSE_SUCCESS;
		   	  break;
		   }
                   
		   int pos_key=header_line.find(':');
		   if(pos_key==string::npos ||header_line.size()-pos_key<3){
		   	  resultState_==PARSE_ERROR;
		   	  break;

		   }
		   string header_key=header_line.substr(0,pos_key);
		   string header_value=header_line.substr(pos_key+2);
		   headers_.insert(pair<string,string>(header_key,header_value));
		  nowReadPos_=pos_end+2;
	    }while(false);

	    if(resultState_==PARSE_ERROR)handleParseError();
	    else if(resultState_==PARSE_SUCCESS){
	//	    cout<<"parse header success\n";
	    	nowReadPos_=pos_end+2;
	    	inBuffer_=inBuffer_.substr(nowReadPos_);
	    	nowReadPos_=0;
	    	processState_=PARSE_FINISH;
		}
	}
}

void HttpData::analysisRequest() {
	//if(requestType_!=GET)cout<<"requestType error\n";
	cout<<"work in parse request file name:"<<fileName_<<endl;
	if(requestType_==GET){
		void *mmapRet;
		struct stat sbuf;
		do{

	        int dot_pos=fileName_.find('.');
		    if (dot_pos==string::npos)
               fileType_ = MimeType::getMime("default");
            else
               fileType_ = MimeType::getMime(fileName_.substr(dot_pos));

          //  cout<<"fileType "<<fileType_<<"\n";
            if (stat(fileName_.c_str(), &sbuf) < 0) {
                resourceState_=RESOURCE_ERROR;
                break;
            }

            int src_fd = open(fileName_.c_str(), O_RDONLY, 0);
            if (src_fd < 0) {
		resourceState_=RESOURCE_ERROR;
                break;
            }

            mmapRet = mmap(NULL, sbuf.st_size, PROT_READ, MAP_PRIVATE, src_fd, 0);
            close(src_fd);
            if (mmapRet == (void *)-1) {
                munmap(mmapRet, sbuf.st_size);
                resourceState_=RESOURCE_ERROR;
		break;
            }
            resourceState_=RESOURCE_SUCCESS;

        }while(false);


        if(resourceState_==RESOURCE_ERROR)handleError(fd_, 404, "Not Found!");
        else{
		cout<<"resource success\n";
        	 string header;
             header += "HTTP/1.1 200 OK\r\n";
             if (headers_.find("Connection") != headers_.end()&& headers_["Connection"] == "Keep-Alive" ) {
                    keepAlive_ = true;
                    header += string("Connection: Keep-Alive\r\n");
		    header+= "Keep-Alive: timeout=";
                    header+=to_string(DEFAULT_KEEP_ALIVE_TIME);
		    header+= "\r\n";
             }
             header += "Content-Type: ";
	     header += fileType_ ;
	     header+= "\r\n";
             header += "Content-Length: " + to_string(sbuf.st_size) + "\r\n";
             header += "\r\n";
	     outBuffer_+=header;
             char *src_addr = static_cast<char *>(mmapRet);
             outBuffer_ += string(src_addr, src_addr + sbuf.st_size);
             munmap(mmapRet, sbuf.st_size);
         }
     }
}

void HttpData::handleError(int fd, int err_num, string short_msg) {
	cout<<"resource error\n";
  short_msg = " " + short_msg;
  char send_buff[4096];
  string body_buff, header_buff;
  body_buff += "<html><title>出错了</title>";
  body_buff += "<body bgcolor=\"ffffff\">";
  body_buff += to_string(err_num) + short_msg;
  body_buff += "<hr><em>  Web Server</em>\n</body></html>";

  header_buff += "HTTP/1.1 " + to_string(err_num) + short_msg + "\r\n";
  header_buff += "Content-Type: text/html\r\n";
  header_buff += "Connection: Close\r\n";
  header_buff += "Content-Length: " + to_string(body_buff.size()) + "\r\n";
  header_buff += "Web Server\r\n";
  header_buff += "\r\n";
  outBuffer_+=header_buff;
  outBuffer_+=body_buff;

}

