#include"EPollPoller.h"
#include"Logger.h"
#include"Channel.h"
#include<errno.h>

//channel 未添加到poller中
const int kNew = -1;
//channel 已添加到poller中
const int KAdded = 1;
// channel 从poller中删除
const int kDeleted = 2;


EPollPoller::EPollPoller(EventLoop *loop)
  :Poller(loop)
   ,epollfd_(::epoll_create1(EPOLL_CLOEXEC))
   ,events_(kInitEventListSize)
{
  if(epollfd_ < 0)
  {
    LOG_FATAL("epoll_create error:%d\n",errno);
  }
}

EPollPoller::~EPollPoller()
{
  ::close(epollfd_);
}

//通过epoll_wait把发生事件的channel告知给EventLoop
Timestamp EPollPoller::poll(int timeoutMs,ChannelList *activeChannels)
{
  //实际上用LOG_DEBUG输出日志更合理，但是为了看到流程还是用INFO
  LOG_INFO("func = %s => fd total count:%d\n",__FUNCTION__,channels_.size());
  
  //events_.size()返回的是size_t,epoll_wait第三个参数需要一个int
  int numEvents = ::epoll_wait(epollfd_,&*events_.begin(),static_cast<int>(events_.size()),timeoutMs);
  
  int saveErrno = errno;
  Timestamp now(Timestamp::now());
  if(numEvents > 0)
  {
    LOG_INFO("%d events happened \n",numEvents);
    fillActiveChannels(numEvents,activeChannels);
    if(numEvents == events_.size()) //发生事件的个数与vector中的数量一样了，扩容
    {
      events_.resize(events_.size()*2);
    }
  }
  else if(numEvents == 0)
  {
    LOG_DEBUG("%s timeout! \n",__FUNCTION__);
  }
  else 
  {
    if(saveErrno != EINTR)
    {
      errno = saveErrno;
      LOG_ERROR("EPollPoller::poll() err!\n");
    }
  }
  return now;
}


//channel update => EventLoop updateChannel  => Poller updateChanne
/*
 *         EventLoop 
 *  ChannelList      Poller 
 *                  ChannelMap <fd,channel*>
 */
void EPollPoller::updateChannel(Channel * channel)
{
  const int index = channel->index();
  LOG_INFO("func = %s fd=%d events=%d index=%d \n",__FUNCTION__,channel->fd(),channel->events(),index);

  if(index ==kNew || index == kDeleted)
  {
    if(index == kNew)
    {
      int fd = channel->fd();
      channels_[fd] = channel;
    }

    channel->set_index(KAdded);
    update(EPOLL_CTL_ADD,channel);
  }
  else //channel已经在poller上注册过了 
  {
    int fd = channel->fd();
    if(channel->isNoneEvent())
    {
      update(EPOLL_CTL_DEL,channel);
      channel->set_index(kDeleted);
    }
    else 
    {
      update(EPOLL_CTL_MOD,channel);
    }
  }
}

//从poller中删除channel
void EPollPoller::removeChannel(Channel * channel)
{
  int fd = channel->fd();
  channels_.erase(fd);

  LOG_INFO("func = %s fd=%d\n",__FUNCTION__,fd);

  int index = channel->index();
  if(index == KAdded)
  {
    update(EPOLL_CTL_DEL,channel);
  }
  channel->set_index(kNew);
}


void  EPollPoller::fillActiveChannels(int numEvents,ChannelList *activeChannels)const
{
  for(int i=0;i<numEvents;++i)
  {
    Channel * channel = static_cast<Channel*>(events_[i].data.ptr);

    channel->set_revents(events_[i].events);
    activeChannels->push_back(channel);//EventLoop拿到它的poller给它返回的所有事件的channel列表中
  }
}


//更新channel通道,epoll_ctl add/mod/del 
void EPollPoller::update(int operation,Channel *channel)
{
  epoll_event event;
  memset(&event,0,sizeof event);

  int fd = channel->fd();

  event.events = channel->events();
  event.data.fd = fd; 
  event.data.ptr = channel; 

  if(::epoll_ctl(epollfd_,operation,fd,&event) < 0)
  {
     if(operation == EPOLL_CTL_DEL)
     {
       LOG_ERROR("epoll_ctl del error:%d\n",errno);
     }
     else 
     {
       LOG_FATAL("epoll_ctl add/mod error:%d\n",errno);
     }
  }
}





