#include <thread>
#include <iostream>

#include "tcp_server.hpp"

int main(int argc, char **argv) {
  if (argc != 2) {
    std::cerr << "not enough arguments\n";
    return 1;
  }
  
  TcpServer server(argv[1]);
  try {
    server.start();
    std::this_thread::sleep_for(std::chrono::seconds(30));
    server.stop();
  } catch(std::exception &e) {
    std::cerr << e.what() << '\n';
  }
} 