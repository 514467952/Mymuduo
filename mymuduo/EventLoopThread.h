#pragma once 

#include"noncopyable.h"
#include"Thread.h"

#include<functional>
#include<mutex>
#include<condition_variable>

class EventLoop; 

class EventLoopThread : noncopyable 
{
public:
  using ThreadInitCallBack = std::function<void(EventLoop*)>;

  EventLoopThread(const ThreadInitCallBack &cb = ThreadInitCallBack()
      , const std::string &name = std::string());
  ~EventLoopThread();

  EventLoop* startloop();

private:
  void threadFunc();

  EventLoop * loop_;
  bool exiting_;
  
  Thread thread_;
  std::mutex mutex_;
  std::condition_variable cond_;
  ThreadInitCallBack callback_;
};
