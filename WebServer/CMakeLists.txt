cmake_minimum_required(VERSION 2.8)
add_executable(Main
	Main.cpp
	Channel.cpp
	Epoll.cpp
	EventLoop.cpp
	EventLoopThread.cpp
	EventLoopThreadPool.cpp
	HttpData.cpp
	Server.cpp
	Timer.cpp
	Util.cpp
	base/AsyncLogging.cpp
	base/CountDownLatch.cpp
	base/FileUtil.cpp
	base/LogFile.cpp
	base/Logging.cpp
	base/Thread.cpp
	)

set(CMAKE_CXX_FLAGS -lnuma)
set(CMAKE_CXX_FLAGS -pthread)
message(STATUS "CMAKE_CXX_FLAGS=${CMAKE_CXX_FLADS}")

