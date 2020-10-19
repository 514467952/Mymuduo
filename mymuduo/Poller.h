#pragma  once 

#include"noncopyable.h"
#include"Timestamp.h"
#include<vector>
#include<unordered_map>

class Channel;
class EventLoop;

//muduo库中多路事件分发器的核心IO复用
class Poller : noncopyable 
{
public:
  using ChannelList = std::vector<Channel*>;

  Poller(EventLoop *Loop);
  virtual ~Poller() = default;

  //给所有IO复用保留统一的接口
  virtual Timestamp poll (int timeoutMs,ChannelList * activeChannels) = 0; //epool_wait
  virtual void updateChannel(Channel * Channel) = 0;//epoll_ctl ADD mod
  virtual void removeChannel(Channel * Channel) = 0;//epoll_ctl DEl
  
  //判断参数channel是否在当前Poller当中
  bool hasChannel(Channel * channel)const;
  
  //EventLoop可以通过该接口获取默认的IO复用的具体实现
  static Poller* newDefaultPoller(EventLoop* loop);
protected:

  //map的key表示sockfd
  using ChannelMap =std::unordered_map<int,Channel*>;
  ChannelMap channels_;
private: 
  EventLoop *ownerLoop_; //定义Poller所属的事件循环EventLoop

};
