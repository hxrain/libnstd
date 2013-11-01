
#include <nstd/File.h>
#include <nstd/Debug.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <cstdio>
#include <cerrno>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#endif

File::File()
{
#ifdef _WIN32
  ASSERT(sizeof(void*) >= sizeof(HANDLE));
  fp = INVALID_HANDLE_VALUE;
#else
  fp = 0;
#endif
}

File::~File()
{
  close();
}

void_t File::close()
{
#ifdef _WIN32
  if(fp != INVALID_HANDLE_VALUE)
  {
    CloseHandle((HANDLE)fp);
    fp = INVALID_HANDLE_VALUE;
  }
#else
  if(fp)
  {
    fclose((FILE*)fp);
    fp = 0;
  }
#endif
}

bool_t File::unlink(const String& file)
{
#ifdef _WIN32
  if(!DeleteFile(file))
    return false;
#else
  if(::unlink(file) != 0)
    return false;
#endif
  return true;
}

bool_t File::open(const String& file, uint_t flags)
{
#ifdef _WIN32
  if(fp != INVALID_HANDLE_VALUE)
  {
    SetLastError(ERROR_INVALID_HANDLE);
    return false;
  }
  DWORD desiredAccess = 0, creationDisposition = 0;
  if(flags & writeFlag)
  {
    desiredAccess |= GENERIC_WRITE;
    creationDisposition |= CREATE_ALWAYS;
  }
  if(flags & readFlag)
  {
    desiredAccess |= GENERIC_READ;
    if(!(flags & writeFlag))
      creationDisposition |= OPEN_EXISTING;
  }
  fp = CreateFile(file, desiredAccess, FILE_SHARE_READ, NULL, creationDisposition, FILE_ATTRIBUTE_NORMAL, NULL);
  if(fp == INVALID_HANDLE_VALUE)
    return false;
#else
  if(fp)
    return false;
  const char_t* mode = (flags & (writeFlag | readFlag)) == (writeFlag | readFlag) ? "w+" : (flags & writeFlag ? "w" : "r");
  fp = fopen(file, mode); // TODO: do not use fopen api
  if(!fp)
    return false;
#endif

  return true;
}

int_t File::read(char_t* buffer, int_t len)
{
#ifdef _WIN32
  DWORD i;
  if(!ReadFile((HANDLE)fp, buffer, len, &i, NULL))
    return 0;
  return i;
#else
  size_t i = fread(buffer, 1, len, (FILE*)fp);
  if(i == 0)
    return 0;
  return (int_t)i;
#endif
}

int_t File::write(const char_t* buffer, int_t len)
{
#ifdef _WIN32
  DWORD i;
  if(!WriteFile((HANDLE)fp, buffer, len, &i, NULL))
    return 0;
  return i;
#else
  size_t i = fwrite(buffer, 1, len, (FILE*)fp);
  return (int_t)i;
#endif
}

bool_t File::write(const String& data)
{
  return write(data, data.length()) == (int)data.length();
}

String File::dirname(const String& file)
{
  const char_t* start = file;
  const char_t* pos = &start[file.length() - 1];
  for(; pos >= start; --pos)
    if(*pos == '\\' || *pos == '/')
      return file.substr(0, (int_t)(pos - start));
  return String(".");
}

String File::basename(const String& file, const String& extension)
{
  const char_t* start = file;
  uint_t fileLen = file.length();
  const char_t* pos = &start[fileLen - 1];
  const char_t* result;
  for(; pos >= start; --pos)
    if(*pos == '\\' || *pos == '/')
    {
      result = pos + 1;
      goto removeExtension;
    }
  result = start;
removeExtension:
  uint_t resultLen = fileLen - (uint_t)(result - start);
  uint_t extensionLen = extension.length();
  if(extensionLen)
  {
    const char_t* extensionPtr = extension;
    if(*extensionPtr == '.')
    {
      if(resultLen >= extensionLen)
        if(String::compare((const char_t*)result + resultLen - extensionLen, extensionPtr) == 0)
          return String(result, resultLen - extensionLen);
    }
    else
    {
      uint_t extensionLenPlus1 = extensionLen + 1;
      if(resultLen >= extensionLenPlus1 && result[resultLen - extensionLenPlus1] == '.')
        if(String::compare((const char_t*)result + resultLen - extensionLen, extensionPtr) == 0)
          return String(result, resultLen - extensionLenPlus1);
    }
  }
  return String(result, resultLen);
}

String File::extension(const String& file)
{
  const char_t* start = file;
  uint_t fileLen = file.length();
  const char_t* pos = &start[fileLen - 1];
  for(; pos >= start; --pos)
    if(*pos == '.')
      return String(pos + 1, fileLen - ((uint_t)(pos - start) + 1));
    else if(*pos == '\\' || *pos == '/')
      return String();
  return String();
}
/*

String File::simplifyPath(const String& path)
{
  String result(path.getLength());
  const char* data = path.getData();
  const char* start = data;
  const char* end;
  String chunck;
  bool startsWithSlash = *data == '/' || *data == '\\';
  for(;;)
  {
    while(*start && (*start == '/' || *start == '\\'))
      ++start;
    end = start;
    while(*end && *end != '/' && *end != '\\')
      ++end;

    if(end == start)
      break;

    chunck = path.substr(start - data, end - start);
    if(chunck == ".." && !result.isEmpty())
    {
      const char* data = result.getData();
      const char* pos = data + result.getLength() - 1;
      for(;; --pos)
        if(pos < data || *pos == '/' || *pos == '\\')
        {
          if(strcmp(pos + 1, "..") != 0)
          {
            if(pos < data)
              result.setLength(0);
            else
              result.setLength(pos - data);
            goto cont;
          }
          break;
        }
    }
    else if(chunck == ".")
      goto cont;

    if(!result.isEmpty() || startsWithSlash)
      result.append('/');
    result.append(chunck);

  cont:
    if(!*end)
      break;
    start = end + 1;
  }
  return result;
}

bool File::isPathAbsolute(const String& path)
{
  const char* data = path.getData();
  return *data == '/' || *data == '\\' || (path.getLength() > 2 && data[1] == ':' && (data[2] == '/' || data[2] == '\\'));
}

bool File::writeTime(const String& file, long long& writeTime)
{
#ifdef _WIN32
  WIN32_FIND_DATAA wfd;
  HANDLE hFind = FindFirstFile(file.getData(), &wfd);
  if(hFind == INVALID_HANDLE_VALUE)
    return false;
  ASSERT(sizeof(DWORD) == 4);
  writeTime = ((long long)wfd.ftLastWriteTime.dwHighDateTime) << 32LL | ((long long)wfd.ftLastWriteTime.dwLowDateTime);
  FindClose(hFind);
  return true;
#else
  struct stat buf;
  if(stat(file.getData(), &buf) != 0)
    return false;
  writeTime = ((long long)buf.st_mtim.tv_sec) * 1000000000LL + ((long long)buf.st_mtim.tv_nsec);
  return true;
#endif
}
*/
bool_t File::exists(const String& file)
{
#ifdef _WIN32
  WIN32_FIND_DATAA wfd;
  HANDLE hFind = FindFirstFile(file, &wfd); // TODO: use GetFileAttribute ?
  if(hFind == INVALID_HANDLE_VALUE)         // I guess GetFileAttribute does not work on network drives. But it will produce
    return false;                           // an error code that can be used to fall back to another implementation.
  FindClose(hFind);
  return true;
#else
  struct stat buf;
  if(lstat(file, &buf) != 0)
    return false;
  return true;
#endif
}