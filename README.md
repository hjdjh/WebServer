[readme.txt](https://github.com/hjdjh/WebServer/files/7020943/readme.txt)
# WebServer
Http  WebServer Reactor Linux C++


      该项目是基于Reactor和线程池的多并发服务器，支持定时器、日志器、和http长链接。每个线程池线程都都有一个任务队列、epoll循环、任务队列，主线程不断循环在accept函数，有新连接到来时 使用轮询的方式 放到一个线程池线程的任务队列。
 
1、任务队列

（1）任务队列是基于queue<std::functor>,通过bind函数传参；

（2）每一个epoll循环都会挂载一个关于任务队列的文件描述符，通过向该文件描述符写数据来唤醒任务队列。


2、日志器

（1）采用STL的优先队列，该优先队列的特点是数据有序，但不可随机读，所以采用shared_ptr来管理定时节点。每个http连接都会有一个定时节点，优先队列上可以有多个定时节点的shared_ptr指针，优先队列会将 到期的或者乱序定时节点shared_ptr指针 去除，如果任务队列里没有某个定时节点的shared_ptr指针，该定时节点的析构函数就会被调用，该析构函数会将对应的shutdown对应http连接的写端。

（2）当有http连接可读或有数据可写的时候，会更新对应定时节点的超时时间，并新建一个该定时节点的shared_ptr指针，将其挂载到定时器的优先队列上。

3、日志器

（1）采用双缓冲的技术，日志器的前端有两个缓冲区，后端线程有两个缓冲区，当定时条件满足或前端缓冲区写满时通过条件变量通知日志器后台线程，后端线程将自身缓冲区和前端缓冲区进行交换，从而减少临界区。

（2）日志器有两个类，一个是LogStream,重载了<<运算符，进行格式化输入，并将日志信息成行；另一个是AsyncLogging静态类，有（1）所说的前端缓冲区，多线程共享。写日志信息时先调用日志器的构造函数，之后调用LogStream <<方法，在日志器析构的时候会添加时间、行号、所在文件名到日志信息上，然后将日志信息写到AsyncLogging的缓冲区上。

4、运行与测试

（1）在WebServer/build目录下运行cmake .. ; make ;./Main -p 2880 

(2)测试方法有两种，一是服务器运行后再浏览器访问Http://127.0.0.1/WebPage/begin.html ;二是在运行WebServer/tests目录下的客户端，该客户端会建立多个http客户端连接多次访问服务器的多个文件，并与原文件进行对比，判断是否出现错误。
