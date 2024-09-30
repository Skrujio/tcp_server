#ifndef TCP_SERVER_HPP
#define TCP_SERVER_HPP

#include <unistd.h>
#include <netdb.h>
#include <poll.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/eventfd.h>

#include <cstring>
#include <thread>
#include <mutex>
#include <fstream>
#include <format>
#include <utility>

#include "thread_pool.hpp"

class TcpServer {
public:
  TcpServer(const char *port) : port{port} {}
  void start();
  void stop();
  
private:
  std::thread server_thread;
  int event_fd{};
  const char *port{};
  
  void server_thread_function();
  int get_socket(const char *port);
};

void TcpServer::start() {
  event_fd = eventfd(0, 0);
  if (event_fd == -1) {
    throw std::runtime_error(std::format("{}: {}", "eventfd", std::strerror(errno)));
  }

  server_thread = std::thread(&TcpServer::server_thread_function, this);
}

void TcpServer::stop() {
  uint64_t one = 1;
  auto rv = write(event_fd, &one, sizeof(uint64_t));
  if (rv == -1) {
    throw std::runtime_error(std::format("{}: {}", "write", std::strerror(errno)));
  }
  
  server_thread.join();
}

void TcpServer::server_thread_function() {
  int sock_fd = get_socket(port);
  
  std::vector<pollfd> fds{
    {event_fd, POLLIN, 0},
    {sock_fd, POLLIN, 0}
  };
  
  ThreadPool thread_pool(4);
  thread_pool.start();
  std::mutex mutex;
  
  std::ofstream log_fs("log.txt", std::ios::app);
  
  while (true) {
    const int timeout = 10'000;
    int n = poll(fds.data(), fds.size(), timeout);
    if (n == -1 && errno != ETIMEDOUT && errno != EINTR) {
      std::perror("poll");
      break;
    }
    
    if (n == 0) {
      continue;
    }
    
    if (fds[0].revents) {
      break;
    }
    
    if (fds[1].revents) {
      int new_fd = accept(sock_fd, nullptr, nullptr);
      if (new_fd == -1) {
        std::perror("accept");
        continue;
      }
      
      thread_pool.enqueue([new_fd, &log_fs, &mutex] {
        const int buffer_size = 256;
        char buffer[buffer_size]{};
        
        while (int rv = recv(new_fd, buffer, buffer_size - 1, 0)) {
          if (rv == -1) {
            std::perror("recv");
          }
          
          {
            std::lock_guard<std::mutex> lock(mutex);
            log_fs.write(buffer, rv);
            log_fs.flush();
          }
        }
        
        close(new_fd);
      });
    }
  }
}
  
int TcpServer::get_socket(const char *port) {
  addrinfo hints{};
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = AI_PASSIVE;
  
  addrinfo *server_info{};
  int rv = getaddrinfo(nullptr, port, &hints, &server_info);
  if (rv != 0) {
    throw std::runtime_error(std::format("getaddrinfo: {}", gai_strerror(rv)));
  }
  
  int sock_fd{};
  addrinfo *p{};
  for (p = server_info; p != nullptr; p = p->ai_next) {
    sock_fd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
    if (sock_fd == -1) {
      std::perror("socket");
      continue;
    }
    
    int yes = 1;
    if (setsockopt(sock_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
      close(sock_fd);
      throw std::runtime_error(std::format("setsocketopt: {}", std::strerror(errno)));
    }
    
    if (bind(sock_fd, p->ai_addr, p->ai_addrlen) == -1) {
      close(sock_fd);
      std::perror("bind");
      continue;
    }
    
    break;
  }
  
  freeaddrinfo(server_info);
  
  if (p == nullptr) {
    throw std::runtime_error("server: failed to bind\n");
  }
  
  const int backlog = 10;
  if (listen(sock_fd, backlog)) {
    close(sock_fd);
    throw std::runtime_error(std::format("listen: {}", std::strerror(errno)));
  }
  
  return sock_fd;
}

#endif