#include"HttpClient.h"
#include"../base/Logging.h"
#include"../EventLoop.h"
#include<string>
#include<stdlib.h>
#include<vector>
#include<sys/stat.h>
#include<unistd.h>
#include<fcntl.h>
#include<memory>
#include<sys/mman.h>
#include<iostream>

#define REPEAT_NUM 10
#define CONNECTION_NUM 200

using namespace std;

vector<string> testFile(10);
vector<string> request(10);
vector<string> verifyData(10);
string verifyDataAll;

void addVerifyData(){

        testFile[0]+="/resourse/testFile0.txt";
        testFile[1]+="/resourse/testFile1.txt";
        testFile[2]+="/resourse/testFile2.txt";
        testFile[3]+="/resourse/testFile3.txt";
        testFile[4]+="/resourse/testFile4.txt";
        testFile[5]+="/resourse/testFile5.txt";
        testFile[6]+="/resourse/testFile6.txt";
        testFile[7]+="/resourse/testFile7.txt";
        testFile[8]+="/resourse/testFile8.txt";
        testFile[9]+="/resourse/testFile9.txt";
        



	request[0]+="GET "+testFile[0]+" HTTP/1.1\r\nContent-Type: text/plain\r\nConnection: Keep-Alive\r\n\r\n";
        request[1]+="GET "+testFile[1]+" HTTP/1.1\r\nContent-Type: text/plain\r\nConnection: Keep-Alive\r\n\r\n";
        request[2]+="GET "+testFile[2]+" HTTP/1.1\r\nContent-Type: text/plain\r\nConnection: Keep-Alive\r\n\r\n";
        request[3]+="GET "+testFile[3]+" HTTP/1.1\r\nContent-Type: text/plain\r\nConnection: Keep-Alive\r\n\r\n";
        request[4]+="GET "+testFile[4]+" HTTP/1.1\r\nContent-Type: text/plain\r\nConnection: Keep-Alive\r\n\r\n";
        request[5]+="GET "+testFile[5]+" HTTP/1.1\r\nContent-Type: text/plain\r\nConnection: Keep-Alive\r\n\r\n";
        request[6]+="GET "+testFile[6]+" HTTP/1.1\r\nContent-Type: text/plain\r\nConnection: Keep-Alive\r\n\r\n";
        request[7]+="GET "+testFile[7]+" HTTP/1.1\r\nContent-Type: text/plain\r\nConnection: Keep-Alive\r\n\r\n";
        request[8]+="GET "+testFile[8]+" HTTP/1.1\r\nContent-Type: text/plain\r\nConnection: Keep-Alive\r\n\r\n";
        request[9]+="GET "+testFile[9]+" HTTP/1.1\r\nContent-Type: text/plain\r\nConnection: Keep-Alive\r\n\r\n";


	for(int i=0;i<verifyData.size();i++){
		struct stat sbuf;
                stat(testFile[i].c_str()+1, &sbuf);

		verifyData[i]=verifyData[i]+"HTTP/1.1 200 OK\r\n"
		               +"Connection: Keep-Alive\r\n"
		               +"Keep-Alive: timeout=300000\r\n"
		               +"Content-Type: text/plain\r\n"
		               +"Content-Length: " + to_string(sbuf.st_size) + "\r\n"
		               +"\r\n";

		int src_fd = open(testFile[i].c_str()+1, O_RDONLY, 0);
		void *mmapRet = mmap(NULL, sbuf.st_size, PROT_READ, MAP_PRIVATE, src_fd, 0);
		char *src_addr = static_cast<char *>(mmapRet);
                verifyData[i]+= string(src_addr, src_addr + sbuf.st_size);
                verifyDataAll+=verifyData[i];

             //   if(i==0) cout<<"verifyData:   "<<verifyData[i]<<"\n\n";
                close(src_fd);
	}
}



int main(int argc,char* argv[]){
	addVerifyData();
	if(argc!=3)return 0;
	string ip(argv[1]);
	int port=atoi(argv[2]);
	EventLoop loop;
        vector<HttpClient*> connManager;
        for(int i=0;i<CONNECTION_NUM;i++){
	     
              connManager.push_back(new HttpClient(&loop,ip,port));
	     
	}

	for(int i=0;i<connManager.size();i++){
               
                string& outBuffer=connManager[i]->getOutBuffer();
                for(int i=0;i<REPEAT_NUM;i++)
                     for(int j=0;j<request.size();j++){
                         outBuffer+=request[j];
                     }
		//cout<<outBuffer<<"\n----------------------------------\n";
		connManager[i]->connection();
		connManager[i]->addToPoller();
	}
	cout<<request[9]<<"\n";
	loop.loop();
}


