
#include <future>
#include <cassert>
#include <cstdio>

void testFutureStd(int iterations)
{
  struct A
  {
    static int64_t testProc(int64_t i)
    {
      return i;
    }
  };

  int64_t result = 0;
  for(int i = 0; i < iterations; ++i)
  {
    std::future<int64_t> future = std::async(&A::testProc, i);
    result += future.get();
  }
  if(result != (int64_t)iterations * (int64_t)(iterations - 1) / 2)
  {
    printf("fail\n");
    std::terminate();
  }
}

void testFutureStd2(int iterations)
{
  struct A
  {
    static int64_t testProc(int64_t i)
    {
      return i;
    }
  };

  int64_t result = 0;
  for(int i = 0; i < iterations; ++i)
  {
    std::future<int64_t> futures[128];
    for(int j = 0; j < 128; ++j)
      futures[j] = std::async(&A::testProc, j);
    for(int j = 0; j < 128; ++j)
      result += futures[j].get();
  }
  if(result != (int64_t)128 * (int64_t)(128 - 1) / 2 * iterations)
  {
    printf("fail\n");
    std::terminate();
  }
}
