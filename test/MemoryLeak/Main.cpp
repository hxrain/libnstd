
#include <nstd/Memory.h>

class A
{
public:
  int a;
};

int_t main(int_t argc, char_t* argv[])
{
  A* a = new A; // leak #1
  A* b = new A; // no leak
  void* c = Memory::alloc(123); // leak #2
  void* d = Memory::alloc(123); // no leak
  
  delete b;
  Memory::free(d);

  return 0;
}