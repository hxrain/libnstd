
#include <cstdarg>
#include <cstdio>

#include <nstd/Log.h>
#include <nstd/Mutex.h>
#include <nstd/Time.h>
#include <nstd/Process.h>
#include <nstd/Thread.h>
#include <nstd/Console.h>
#include <nstd/Debug.h>

class _Log
{
public:
  static Mutex mutex;
  static String lineFormat;
  static String timeFormat;
  static int level;

  static void vlogf(int level, const tchar* format, va_list& vl)
  {
    int64 time = Time::time();
    String lineFormat;
    String timeFormat;
    _Log::mutex.lock();
    lineFormat = _Log::lineFormat;
    timeFormat = _Log::timeFormat;
    _Log::mutex.unlock();

    // get message
    String data(200);
    int result;
    {
      usize capacity = data.capacity();
#ifdef _UNICODE
      result = _vsnwprintf((wchar_t*)data, capacity, format, vl);
#else
      result = vsnprintf((char*)data, capacity, format, vl);
#endif
      if(result >= 0 && result < (int)capacity)
        data.resize(result);
      else // buffer was too small: compute size, reserve buffer, print again
      {
#ifdef _MSC_VER
#ifdef _UNICODE
        result = _vscwprintf(format, vl);
#else
        result = _vscprintf(format, vl);
#endif
#else
        result = vsnprintf(0, 0, format, vl);
#endif
        ASSERT(result >= 0);
        if(result >= 0)
        {
          data.reserve(result);
      #ifdef _UNICODE
          result = _vsnwprintf((wchar_t*)data, result + 1, format, vl);
      #else
          result = vsnprintf((char*)data, result + 1, format, vl);
      #endif
          ASSERT(result >= 0);
          if(result >= 0)
            data.resize(result);
        }
      }
    }

    // build line
    String line(data.length() + 200);
    for(const tchar* p = lineFormat; *p; ++p)
    {
      if(*p == _T('%'))
      {
        ++p;
        switch(*p)
        {
        case _T('%'):
          line += _T('%');
          break;
        case _T('m'):
          line += data;
          break;
        case _T('t'):
          line += Time::toString(time, timeFormat);
          break;
        case _T('L'):
          switch(level)
          {
          case Log::debug: line += _T("debug"); break;
          case Log::info: line += _T("info"); break;
          case Log::warning: line += _T("warning"); break;
          case Log::error: line += _T("error"); break;
          case Log::critical: line += _T("critical"); break;
          default: line += _T("unknown"); break;
          }
          break;
        case _T('P'):
          line += String::fromInt(Process::getCurrentProcessId());
          break;
        case _T('T'):
          line += String::fromInt(Thread::getCurrentThreadId());
          break;
        default:
          line += _T('%');
          line += *p;
          break;
        }
      }
      else
        line += *p;
    }
    line += _T('\n');
    if(level >= Log::warning)
      Console::error(line);
    else
      Console::print(line);
  }

};

Mutex _Log::mutex;
String _Log::lineFormat(_T("[%t] %L: %m"));
String _Log::timeFormat(_T("%H:%M:%S"));
int _Log::level = Log::info;

void Log::setFormat(const String& lineFormat, const String& timeFormat)
{
  _Log::mutex.lock();
  _Log::lineFormat = lineFormat;
  _Log::timeFormat = timeFormat;
  _Log::mutex.unlock();
}

void Log::setLevel(int level)
{
  _Log::level = level;
}

void Log::logf(int level, const tchar* format, ...)
{
  if(level < _Log::level)
    return;
  va_list vl;
  va_start(vl, format);
  _Log::vlogf(level, format, vl);
  va_end(vl);
}

void Log::infof(const tchar* format, ...)
{
  if(Log::info < _Log::level)
    return;
  va_list vl;
  va_start(vl, format);
  _Log::vlogf(Log::info, format, vl);
  va_end(vl);
}

void Log::warningf(const tchar* format, ...)
{
  if(Log::warning < _Log::level)
    return;
  va_list vl;
  va_start(vl, format);
  _Log::vlogf(Log::warning, format, vl);
  va_end(vl);
}

void Log::errorf(const tchar* format, ...)
{
  if(Log::error < _Log::level)
    return;
  va_list vl;
  va_start(vl, format);
  _Log::vlogf(Log::error, format, vl);
  va_end(vl);
}
