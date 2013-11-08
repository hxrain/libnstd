
#pragma once

#include <nstd/Memory.h>

template <typename T> class Array
{
public:
  class Iterator
  {
  public:
    Iterator() : item(0) {}
    const T& operator*() const {return *item;}
    const Iterator& operator++() {++item; return *this;}
    const Iterator& operator--() {--item; return *this;}
    bool operator==(const Iterator& other) const {return item == other.item;}
    bool operator!=(const Iterator& other) const {return item != other.item;}

  private:
    T* item;
    
    Iterator(T* item) : item(item) {}

    friend class Array;
  };

  Array() : capacity(0) {}

  ~Array()
  {
    if(_begin.item)
    {
      for(T* i = _begin.item, * end = _end.item; i != end; ++i)
        i->~T();
      Memory::free(_begin.item);
    }
  }

  const Iterator& begin() const {return _begin;}
  const Iterator& end() const {return _end;}

  size_t size() const {return _end.item - _begin.item;}
  bool_t isEmpty() const {return _begin.item == _end.item;}

  void_t clear()
  {
    if(_begin.item)
    {
      for(T* i = _begin.item, * end = _end.item; i != end; ++i)
        i->~T();
      _end.item = _begin.item;
    }
  }

  T& append(const T& value)
  {
    size_t size = _end.item - _begin.item;
    if(size >= capacity)
    {
      size_t bsize;
      T* newData = (T*)Memory::alloc(sizeof(T) * (size + 1), bsize);
      capacity = bsize / sizeof(T);
      T* dest = newData;
      for(T* src = _begin.item, * end = _end.item; src != end; ++src, ++dest)
      {
        VERIFY(new(dest) T(*src) == dest);
        src->~T();
      }
      Memory::free(_begin.item);
      _begin.item = newData;
      _end.item = dest;
    }
    T* item = _end.item;
    VERIFY(new(item) T(value) == item);
    ++_end.item;
    return *item;
  }

  void_t remove(size_t index)
  {
    size_t size = _end.item - _begin.item;
    if(index < size)
    {
      T* pos = _begin.item + index;
      for(T* end = --_end.item, * dest; pos < end;)
      {
        dest = pos;
        *dest = *(++pos);
      }
      pos->~T();
    }
  }

  Iterator remove(const Iterator& it)
  {
    T* pos = it.item;
    for(T* end = --_end.item, * dest; pos < end;)
    {
      dest = pos;
      *dest = *(++pos);
    }
    pos->~T();
    return it.item;
  }

  Iterator find(const T& value)
  {
    for(T* pos = _begin.item, * end = _end.item; pos < end; ++pos)
      if(*pos == value)
        return pos;
    return _end;
  }

private:
  Iterator _begin;
  Iterator _end;
  size_t capacity;
};

