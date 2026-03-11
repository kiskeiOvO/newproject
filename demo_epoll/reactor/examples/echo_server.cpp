#include "net/EventLoop.h"
#include "net/TcpServer.h"

#include <iostream>
#include <thread>

namespace {
const uint16_t kPort = 8888;
const int kListenBacklog = 128;
}
/*
               主线程
            ┌────────────┐
            │ mainLoop   │
            │ accept     │
            └─────┬──────┘
                  │
             新连接 fd
                  │
        EventLoopThreadPool
      ┌─────────┬─────────┬─────────┐
      ▼         ▼         ▼
   worker1   worker2   worker3
   loop1     loop2     loop3
   epoll     epoll     epoll
*/
int main() {
    try {
        EventLoop mainLoop;
        std::cout<<std::thread::hardware_concurrency()<<std::endl;
        int workerThreads = static_cast<int>(std::thread::hardware_concurrency());
        if (workerThreads <= 0) {
            workerThreads = 4;
        }

        TcpServer server(&mainLoop, kPort, kListenBacklog, workerThreads);
        server.start();

        std::cout << "main Reactor listening on " << kPort
                  << ", worker threads=" << workerThreads << "\n";

        mainLoop.loop();
    } catch (const std::exception& ex) {
        std::cerr << "fatal error: " << ex.what() << "\n";
        return 1;
    }

    return 0;
}
