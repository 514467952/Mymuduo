#include"EventLoop.h"
#include"Logger.h"
#include"Poller.h"
#include"Channel.h"

#include<sys/eventfd.h> 
#include<unistd.h>
#include<fcntl.h>
#include<errno.h>
#include<memory>

//防止一个线程创建多个EventLoop对象
__thread EventLoop*t_loopInThisThread = nullptr;

//定义默认的Poller IO复用的超时时间
const int kPollTimeMs = 10000;

//创建wakeupfd,用来notify唤醒subReactor处理新来的channel
int createEventfd()
{
  int evtfd = ::eventfd(0,EFD_NONBLOCK | EFD_CLOEXEC);
  if(evtfd < 0)
  {
    LOG_FATAL("eventfd error:%d \n",errno);
  }
}

//构造函数
EventLoop::EventLoop()
  :looping_(false)
   ,quit_(false)
   ,callingPendingFunctors_(false)
   ,threadId_(CurrentThread::tid())
   ,poller_(Poller::newDefaultPoller(this))
   ,wakeupFd_(createEventfd())
   ,wakeupChannel_(new Channel(this,wakeupFd_))
{
  LOG_DEBUG("EventLoop created %p in thread %d \n",this,threadId_);
  if(t_loopInThisThread)
  {
    LOG_FATAL("Another EventLoop %p  exits in this thread %d \n",t_loopInThisThread,threadId_);
  }
  else 
  {
    t_loopInThisThread = this;
  }

  //设置wakeupfd得事件类型以及发生事件后得回调操作
  wakeupChannel_->setReadCallback(std::bind(&EventLoop::handleRead,this));

  //每一个eveentloop都将监听wakeupchannel得EPOLLIN事件了
  wakeupChannel_->enableReading();
}

EventLoop::~EventLoop()
{
  //资源回收
  wakeupChannel_->disableAll(); //对所有事件不感兴趣
  wakeupChannel_->remove();
  ::close(wakeupFd_);
  t_loopInThisThread = nullptr;
}

//开启事件循环
void EventLoop::loop()
{
  looping_ = true;
  quit_ = false;

  LOG_INFO("EventLoop %p start looping \n",this);

  while(!quit_)
  {
    activeChannels_.clear();

    //监听两类fd，一种是clientfd，一种是wakeupfd
    pollReturnTime_ = poller_->poll(kPollTimeMs,&activeChannels_);
    for(Channel * channel : activeChannels_)
    {
      //Poller能够监听哪些channel发生事件，然后上报给EventLoop，通知Channel处理相应得事件
      channel->handleEvent(pollReturnTime_);
    }

    //执行当前EventLoop事件循环需要处理得回调操作
    /*
     * IO 线程 mainLoop 主要得工作是accept得工作会返回一个新用户得fd
     *已建立连接得channel 会分发给subloop
     * mainloop事先注册一个回调cb(需要subloop来执行)  
     * wakeup  subloop后，执行下面得方法，执行mainloop注册的操作
     */
    doPendingFunctors();

  }

  LOG_INFO("EventLoop  %p stop looping \n",this);
  looping_ = false;
}

//退出事件循环
//1. loop在自己的线程中调用自己的quit
/*
 *    mainloop
 *  
 *  ===  生产者与消费者模型 
 *subloop1  subloop2  subloop3
 */ 
void EventLoop::quit()
{
  quit_ = true;
  
  if(!isInLoopThread()) //如果在其它线程中，调用的quit 在一个subloop中，调用mainloop的quit
  {
    wakeup();
  }
}

//在当前loop中执行cb
void EventLoop::runInLoop(Functor cb)
{
  if(isInLoopThread()) //在当前的loop线程中，执行cb
  {
    cb();
  }
  else //在非当前loop线程中执行cb 唤醒loop所在线程，执行cb
  {
    queueInLoop(cb);
  }
}

//把cb放到队列中，不在当前的loop中执行
void EventLoop::queueInLoop(Functor cb)
{
  {
    std::unique_lock<std::mutex> lock(mutex_);
    pendingFunctors_.emplace_back(cb); //emplace_back直接构造，push_back拷贝构造
  }

  //唤醒相应的需要执行上面回调操作的loop线程了
  //||callingPendingFunctors_是指当前loop正在执行回调，没有阻塞在loop上,但
  //loop又有新的回调
  if(!isInLoopThread() || callingPendingFunctors_)
  {
    wakeup(); //唤醒loop所在线程
  }
}

void EventLoop::handleRead()
{
  uint64_t one = 1;
  ssize_t n = read(wakeupFd_,&one,sizeof one);
  if(n != sizeof one)
  {
    LOG_ERROR("EventLoop::handleRead() reads %ld bytes instead of 8",n);
  }
}


//用来唤醒loop所在的线程的
//向wakeupfd写一个数据
//wakeupChannel就发生读事件，当前loop线程就会被唤醒
void EventLoop::wakeup()
{
  uint64_t one = 1;
  ssize_t n = write(wakeupFd_,&one,sizeof one);
  if(n != sizeof one)
  {
    LOG_ERROR("EventLoop::wakeup() write %lu bytes instead of 8 \n",n);
  }
}

void EventLoop::updateChannel(Channel *channel)
{
  poller_->updateChannel(channel);
}

void EventLoop::removeChannel(Channel *channel)
{
  poller_->removeChannel(channel);
}

bool EventLoop::hasChannel(Channel *channel)
{
  return poller_->hasChannel(channel);
}

//执行回调
void EventLoop::doPendingFunctors()
{
  std::vector<Functor> functors;
  callingPendingFunctors_ = true;

  {
    std::unique_lock<std::mutex> lock(mutex_);
    functors.swap(pendingFunctors_);
  }

  for(const Functor &functor:functors)
  {
    functor();  //执行当前loop需要执行的回调操作
  }

  callingPendingFunctors_  = false;
}









