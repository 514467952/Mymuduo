#pragma once 

#include<unistd.h>
#include<sys/syscall.h>

namespace  CurrentThread
{
  extern __thread int t_cachedTid;

  void cacheTid();

  inline int tid()
  {
    //__builtin_expect是为了提高cpu的执行效率
    if(__builtin_expect(t_cachedTid==0,0))
    {
      cacheTid();
    }
    return t_cachedTid;
  }
}

