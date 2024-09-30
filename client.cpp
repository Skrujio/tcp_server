#include <iostream>

#include "tcp_client.hpp"

int main(int argc, char **argv) {
  if (argc != 4) {
    std::cerr << "not enough arguments\n";
    return 1;
  }
  
  const char *name = argv[1];
  const char *port = argv[2];
  const char *duration = argv[3];
  
  TcpClient client(name, port, duration);
  try {
    client.start();
  } catch(std::exception &e) {
    std::cerr << e.what() << '\n';
  }
}