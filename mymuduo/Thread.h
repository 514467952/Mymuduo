#pragma once 
#include"noncopyable.h"
#include<functional>
#include<thread>
#include<memory>
#include<unistd.h>
#include<string>
#include<atomic>

class Thread : noncopyable 
{

public:
  using ThreadFunc = std::function<void()>;

  explicit Thread(ThreadFunc func ,const std::string& name);
  ~Thread();
  void start();
  void join();

  bool started() const {return started_;}
  pid_t tid() const {return tid_;}
  const std::string& name () const {return name_;}

  static int numCreated(){return numCreated_;}

private:
  void setDefaultName();
private:
  bool started_;
  bool joined_;
  
  std::shared_ptr<std::thread> thread_; 
  pid_t tid_;
  ThreadFunc func_; //线程执行函数 
  std::string name_; //线程的名称

  static std::atomic_int numCreated_; //线程创建的 个数
};



