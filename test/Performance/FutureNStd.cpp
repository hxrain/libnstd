
#include <nstd/Future.h>
#include <nstd/Console.h>
#include <nstd/Process.h>

void testFutureNStd(int iterations)
{
  struct A
  {
    static int64 testProc(int64 i)
    {
      return i;
    }
  };

  int64 result = 0;
  for(int i = 0; i < iterations; ++i)
  {
    Future<int64> future;
    future.start(&A::testProc, i);
    result += future;
  }
  if(result != (int64)iterations * (int64)(iterations - 1) / 2)
  {
    Console::printf("fail\n");
    Process::exit(1);
  }
}

void testFutureNStd2(int iterations)
{
  struct A
  {
    static int64 testProc(int64 i)
    {
      return i;
    }
  };

  int64 result = 0;
  for(int i = 0; i < iterations; ++i)
  {
    Future<int64> futures[128];
    for(int j = 0; j < 128; ++j)
      futures[j].start(&A::testProc, j);
    for(int j = 0; j < 128; ++j)
      result += futures[j];
  }
  if(result != (int64)128 * (int64)(128 - 1) / 2 * iterations)
  {
    Console::printf("fail\n");
    Process::exit(1);
  }
}
