
template <class T> class SharedPtr
{

  SharedPtr(T* obj) : data(new Data)
  {
    data->refs = 1;
    data->obj = obj;
    data->firstWeakPtr = 0;
  }
  
  ~SharedPtr()
  {
    if(Atmoic::decrement(data->refs) == 0)
    {
        for(WeakPtr<T>* i = firstWeakPtr; i; i = i->next)
        {
          i->clear();
        }
        delete data;
    }
  }

private:
  struct Data
  {
    T* obj;
    WeakPtr<T>* firstWeakPtr;
  };

private:
  Data* data;
};

template <class T> class WeakPtr
{
};
