#ifndef THREAD_POOL_HPP
#define THREAD_POOL_HPP

#include <thread>
#include <mutex>
#include <condition_variable>
#include <vector>
#include <queue>
#include <functional>

class ThreadPool {
public:
  ThreadPool(int num_threads = std::thread::hardware_concurrency())
    : workers(num_threads) {}
    
  ~ThreadPool() {
    if (is_running) {
      stop();
    }
  }
  
  void start();
  void enqueue(const std::function<void()> &job);
  void join();
  void stop();
  
private:
  std::vector<std::thread> workers;
  std::queue<std::function<void()>> jobs;
  std::mutex mutex;
  std::condition_variable condition_variable;
  bool is_running = true;
  
  void thread_loop();
  
  ThreadPool(ThreadPool &) = delete;
  ThreadPool(const ThreadPool &) = delete;
  ThreadPool &operator=(ThreadPool &) = delete;
  ThreadPool &operator=(const ThreadPool &) = delete;
};

void ThreadPool::start() {
  for (auto &worker : workers) {
    worker = std::thread(&ThreadPool::thread_loop, this);
  }
}

void ThreadPool::thread_loop() {
  while (true) {
    std::function<void()> job;
    
    {
      std::unique_lock<std::mutex> lock(mutex);
      condition_variable.wait(lock, [this] {
        return !is_running || !jobs.empty();
      });
      
      if (!is_running && jobs.empty()) {
        return;
      }
      
      job = jobs.front();
      jobs.pop();
    }
    
    job();
  }
}

void ThreadPool::enqueue(const std::function<void()> &job) {
  {
    std::unique_lock<std::mutex> lock(mutex);
    jobs.push(job);
  }
  condition_variable.notify_one();
}

void ThreadPool::stop() {
  {
    std::unique_lock<std::mutex> lock(mutex);
    is_running = false;
  }
  condition_variable.notify_all();

  for (auto &worker : workers) {
    worker.join();
  }
}

// template<typename T, typename... Args>
// void ThreadPool::enqueue(T f, Args... args) {
  
// }

#endif