#include<string>
#include<unistd.h>
#include<memory>
class Channel;
class EventLoop;
#define MAXSIZE 1024
#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 8888
#define FDSIZE 1024
#define EPOLLEVENTS 20

class HttpClient{
	private:
		int serverPort_;
		std::string serverIP_;
		bool CONNECTED;
		bool ERROR;
		
		int fd_;
		std::shared_ptr<Channel> channel_;
		void handleRead();
		void handleWrite();
		void handleConn();
		
		EventLoop *loop_;
		std::string inBuffer_;
		std::string outBuffer_;
		
	public:
		HttpClient(EventLoop *loop,std::string ip,int port);
		~HttpClient();
		void connection();
		void addToPoller();
		std::string& getOutBuffer(){ return outBuffer_; } 
		void close();
		
		
		
};
