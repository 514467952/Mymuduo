#include "Acceptor.h"
#include"Logger.h"
#include"InetAddress.h"

#include<sys/types.h>
#include<sys/socket.h>
#include<arpa/inet.h>
#include<errno.h>
#include<unistd.h>

static int createNoblocking()
{
  int sockfd = ::socket(AF_INET,SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC,IPPROTO_TCP);
  if(sockfd < 0)
  {
    LOG_FATAL("%s:%s:%d listen socket create err:%d \n",__FILE__,__FUNCTION__ , __LINE__ ,errno);
  }
  return sockfd;
}

Acceptor::Acceptor(EventLoop* loop,const InetAddress& listenAddr,bool reuseport)
  :loop_(loop)
   , acceptSocket_(createNoblocking())  //创建套接字
   , acceptChannel_(loop,acceptSocket_.fd())
   , listenning_(false)
{
  acceptSocket_.setReuseAddr(true);
  acceptSocket_.setReusePort(true);
  acceptSocket_.bindAddress(listenAddr); //绑定套接字

  //TcpServer::start() Acceptor.listen 有新用户连接，要执行一个回调(connfd,fun) 打包成channel 唤醒一个线程由这个subloop监听新的用户的连接
  // baseloop => acceptChannel_ (listenfd)
  //绑定回调
  acceptChannel_.setReadCallback(std::bind(&Acceptor::headleRead,this));
}

Acceptor::~Acceptor()
{
  acceptChannel_.disableAll();
  acceptChannel_.remove();

}


void Acceptor::listen()
{
  listenning_ = true;
  acceptSocket_.listen(); //启动监听
  acceptChannel_.enableReading(); // acceptChannel_ 注册到poller里
}

//当Channel收到读事件后，就是listenfd有事件发生了，就是有新用户连接了
void Acceptor::headleRead()
{
  InetAddress peerAddr;
  int connfd = acceptSocket_.accept(&peerAddr); //轮询找到subloop ,唤醒分发当前的新客户端的Channel
  if(connfd >= 0)
  {
    if(newConnectionCallback_)
    {
      newConnectionCallback_(connfd,peerAddr);
    }
    else 
    {
      ::close(connfd);
    }

  }
  else 
  {
    LOG_ERROR("%s:%s:%d accept  err:%d \n",__FILE__,__FUNCTION__ , __LINE__ ,errno);
   if(errno == EMFILE) 
   {
    LOG_ERROR("%s:%s:%d sockfd reached limit\n",__FILE__,__FUNCTION__ , __LINE__ );
   }

  }
}

