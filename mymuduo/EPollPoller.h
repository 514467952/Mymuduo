#pragma  once 
#include"Poller.h"
#include"Timestamp.h"

#include<vector>
#include<sys/epoll.h>
#include<unistd.h>
#include<string.h>

class Channel;
/*
 * epoll的使用
 * epoll_create
 * epoll_ctl add/mod/del 
 * epoll_wait 
 */

class EPollPoller: public Poller 
{
public:
  EPollPoller(EventLoop *loop);
  //overrid让编译器检查基类中有没有相应的虚函数
  ~EPollPoller() override;
  
  //重写基类的Poller的抽象方法
  Timestamp poll(int timeoutMs,ChannelList *activeChannels) override;
  void updateChannel(Channel *channel) override;
  void removeChannel(Channel* channel)override ;


private:
  static const int kInitEventListSize = 16;
  
  //填写活跃的连接
  void fillActiveChannels(int numEvents,ChannelList* activeChannels)const;
  //更新channel通道，epoll_ctl
  void update(int operation,Channel* channel);
private:

  using EventList = std::vector<epoll_event>;

  int epollfd_;
  EventList events_;
};
