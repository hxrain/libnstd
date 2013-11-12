
#ifdef _WIN32
#include <Windows.h>
#else
#include <pthread.h>
#endif

#include <nstd/Mutex.h>
#include <nstd/Debug.h>

Mutex::Mutex()
{
#ifdef _WIN32
  ASSERT(sizeof(data) >= sizeof(CRITICAL_SECTION));
  InitializeCriticalSection(&(CRITICAL_SECTION&)data);
#else
  ASSERT(sizeof(data) >= sizeof(pthread_mutex_t));
  pthread_mutexattr_t attr; // TODO: use global var for this?
  pthread_mutexattr_init(&attr);
  pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
  VERIFY(pthread_mutex_init(&(pthread_mutex_t&)data, &attr) == 0);
#endif
}

Mutex::~Mutex()
{
#ifdef _WIN32
  DeleteCriticalSection(&(CRITICAL_SECTION&)data);
#else
  pthread_mutex_destroy(&(pthread_mutex_t&)data);
#endif
}

void_t Mutex::lock()
{
#ifdef _WIN32
  EnterCriticalSection(&(CRITICAL_SECTION&)data);
#else
  VERIFY(pthread_mutex_lock(&(pthread_mutex_t&)data) == 0);
#endif
}

bool_t Mutex::tryLock()
{
#ifdef _WIN32
  return TryEnterCriticalSection(&(CRITICAL_SECTION&)data) == TRUE;
#else
  return pthread_mutex_trylock(&(pthread_mutex_t&)data) == 0;
#endif
}

void_t Mutex::unlock()
{
#ifdef _WIN32
  LeaveCriticalSection(&(CRITICAL_SECTION&)data);
#else
  VERIFY(pthread_mutex_lock(&(pthread_mutex_t&)data) == 0);
#endif
}
