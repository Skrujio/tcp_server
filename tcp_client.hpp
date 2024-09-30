#ifndef TCP_CLIENT_HPP
#define TCP_CLIENT_HPP

#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

#include <cstring>
#include <thread>
#include <chrono>

class TcpClient {
public:
  TcpClient(const char *name, const char *port, const char *duration)
    : name{name}, port{port}, duration{duration} {}
  
  void start();
  
private:
  const char *name;
  const char *port;
  const char *duration;

  int get_socket(const char *port);
};

void TcpClient::start() {
  int sock_fd = get_socket(port);
  
  auto now = std::chrono::system_clock::now;
  auto start = now();
  auto end = start + std::chrono::seconds(std::atoi(duration));
  
  std::cout << std::format("{}\n", now());
  
  for (auto curr = start; curr < end; curr = now()) {
    auto ymd = std::chrono::year_month_day{std::chrono::floor<std::chrono::days>(curr)};
    auto hms = std::chrono::hh_mm_ss{curr - std::chrono::floor<std::chrono::days>(curr)};
    
    std::string msg = std::format("{} {} {}\n", ymd, hms, name);
    int bytes = send(sock_fd, msg.c_str(), msg.length(), 0);
    if (bytes == -1) {
      close(sock_fd);
      throw std::runtime_error(std::format("{}: {}", "send", std::strerror(errno)));
    }
    #include <iostream>
    std::cout.write(msg.c_str(), msg.length());
    
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
  }
  
  close(sock_fd);
}

int TcpClient::get_socket(const char *port) {
  addrinfo hints{};
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_protocol = IPPROTO_TCP;
  
  addrinfo *server_info{};
  int rv = getaddrinfo("localhost", port, &hints, &server_info);
  if (rv != 0) {
    throw std::runtime_error(std::format("{}: {}", "getaddrinfo", gai_strerror(rv)));
  }
  
  int sock_fd{};
  addrinfo *p{};
  for (p = server_info; p != nullptr; p = p->ai_next) {
    sock_fd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
    if (sock_fd == -1) {
      perror("socket");
      continue;
    }
    
    if (connect(sock_fd, p->ai_addr, p->ai_addrlen) == -1) {
      close(sock_fd);
      perror("connect");
      continue;
    }
    
    break;
  }
  
  freeaddrinfo(server_info);
  
  if (p == nullptr) {
    throw std::runtime_error("Failed to connect to server");
  }
  
  return sock_fd;
}

#endif