#include"EventLoopThreadpool.h"
#include"EventLoopThread.h"

#include<memory>
EventLoopThreadPool::EventLoopThreadPool(EventLoop* baseloop,const std::string &nameArg)
  :baseLoop_(baseloop)
   , name_(nameArg)
   , started_(false)
   , numThreads_(0)
   , next_(0)
{}

EventLoopThreadPool::~EventLoopThreadPool()
{}


void EventLoopThreadPool::start(const ThreadInitCallBack &cb)
{
  started_ = true;
  for(int i = 0 ;i<numThreads_;++i)
  {
    char buf[name_.size()+32];
    snprintf(buf,sizeof buf,"%s%d",name_.c_str(),1);

    EventLoopThread *t = new EventLoopThread(cb,buf);
    threads_.push_back(std::unique_ptr<EventLoopThread>(t));
    loops_.push_back(t->startloop());//底层创建线程，绑定一个Eventloop,并返回一个loop的地址

  }

  //整个服务端只有一个线程，运行着baseloop
  if (numThreads_ == 0 && cb)
  {
    cb(baseLoop_);
  }
}

//如果工作在多线程中,baseLoop_默认以轮询的方式分配channel给subloop
EventLoop* EventLoopThreadPool::getNextLoop()
{
  EventLoop* loop = baseLoop_;

  if(!loops_.empty())//通过轮询获取下一个处理事件的loop
  {
    loop = loops_[next_];
    ++next_;
    if((unsigned int)next_ >= loops_.size())
    {
      next_ = 0;
    }
  }
  return loop;
}

std::vector<EventLoop*> EventLoopThreadPool::getAllloops()
{
  if(loops_.empty())
  {
    return std::vector<EventLoop*>(1,baseLoop_); 
  }
  else 
  {
    return loops_;
  }
}
