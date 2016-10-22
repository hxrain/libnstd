
#pragma once

#include <nstd/String.h>
#include <nstd/HashMap.h>
#include <nstd/List.h>

class Variant
{
public:
  enum Type
  {
    nullType,
    boolType,
    doubleType,
    intType,
    uintType,
    int64Type,
    uint64Type,
    mapType,
    listType,
    stringType,
  };

  Variant() : data(&nullData) {}
  ~Variant() {clear();}

  Variant(const Variant& other)
  {
    if(other.data->ref)
    {
      data = other.data;
      Atomic::increment(data->ref);
    }
    else switch(other.data->type)
    {
      case boolType:
        _data.type = boolType;
        _data.ref = 0;
        _data.data.boolData = other.data->data.boolData;
        data = &_data;
        break;
      case intType:
      case uintType:
        _data.type = other.data->type;
        _data.ref = 0;
        _data.data.intData = other.data->data.intData;
        data = &_data;
        break;
      case doubleType:
      case int64Type:
      case uint64Type:
        _data.type = other.data->type;
        _data.ref = 0;
        _data.data.int64Data = other.data->data.int64Data;
        data = &_data;
        break;
#ifdef ASSERT
      case mapType:
      case listType:
      case stringType:
        ASSERT(false);
        // no break
#endif
      default:
        data = &nullData;
        break;
    }
  }

  Variant(bool val) : data(&_data) {_data.type = boolType; _data.ref = 0; _data.data.boolData = val;}
  Variant(double val) : data(&_data) {_data.type = doubleType; _data.ref = 0; _data.data.doubleData = val;}
  Variant(int val) : data(&_data) {_data.type = intType; _data.ref = 0; _data.data.intData = val;}
  Variant(uint val) : data(&_data) {_data.type = uintType; _data.ref = 0; _data.data.uintData = val;}
  Variant(int64 val) : data(&_data) {_data.type = int64Type; _data.ref = 0; _data.data.int64Data = val;}
  Variant(uint64 val) : data(&_data) {_data.type = uint64Type; _data.ref = 0; _data.data.uint64Data = val;}

  Variant(const HashMap<String, Variant>& val)
  {
    data = (Data*)Memory::alloc(sizeof(Data) + sizeof(HashMap<String, Variant>));
    HashMap<String, Variant>* map = (HashMap<String, Variant>*)(data + 1);
    new (map) HashMap<String, Variant>(val);
    data->type = mapType;
    data->ref = 1;
  }

  Variant(const List<Variant>& val)
  {
    data = (Data*)Memory::alloc(sizeof(Data) + sizeof(List<Variant>));
    List<Variant>* list = (List<Variant>*)(data + 1);
    new (list) List<Variant>(val);
    data->type = listType;
    data->ref = 1;
  }

  Variant(const String& val)
  {
    data = (Data*)Memory::alloc(sizeof(Data) + sizeof(String));
    String* string = (String*)(data + 1);
    new (string) String(val);
    data->type = stringType;
    data->ref = 1;
  }

  void clear()
  {
    if(data->ref && Atomic::decrement(data->ref) == 0)
    {
      switch(data->type)
      {
      case mapType: ((HashMap<String, Variant>*)(data + 1))->~HashMap<String, Variant>(); break;
      case listType: ((List<Variant>*)(data + 1))->~List<Variant>(); break;
      case stringType: ((String*)(data + 1))->~String(); break;
      default: break;
      }
      Memory::free(data);
    }
    data = &nullData;
  }

  Variant& operator=(const Variant& other)
  {
    clear();
    if(other.data->ref)
    {
      data = other.data;
      Atomic::increment(data->ref);
    }
    else switch(other.data->type)
    {
      case boolType:
        _data.type = boolType;
        _data.ref = 0;
        _data.data.boolData = other.data->data.boolData;
        data = &_data;
        break;
      case intType:
      case uintType:
        _data.type = other.data->type;
        _data.ref = 0;
        _data.data.intData = other.data->data.intData;
        data = &_data;
        break;
      case doubleType:
      case int64Type:
      case uint64Type:
        _data.type = other.data->type;
        _data.ref = 0;
        _data.data.int64Data = other.data->data.int64Data;
        data = &_data;
        break;
#ifdef ASSERT
      case mapType:
      case listType:
      case stringType:
        ASSERT(false);
        // no break
#endif
      default:
        data = &nullData;
        break;
    }
    return *this;
  }

  Type getType() const {return data->type;}

  bool isNull() const {return data->type == nullType;}

  bool toBool() const
  {
    switch(data->type)
    {
    case boolType: return data->data.boolData;
    case doubleType: return data->data.doubleData != 0.;
    case intType: return data->data.intData != 0;
    case uintType: return data->data.uintData != 0;
    case int64Type: return data->data.int64Data != 0;
    case uint64Type: return data->data.uint64Data != 0;
    case stringType:
      {
        const String& string = *(const String*)(data + 1);
        return string == _T("true") || string == _T("1");
      }
    default:
      return false;
    }
  }

  Variant& operator=(bool other)
  {
    if(data->type != boolType)
    {
      clear();
      data = &_data;
      _data.type = boolType;
      _data.ref = 0;
    }
    _data.data.boolData = other;
    return *this;
  }

  double toDouble() const
  {
    switch(data->type)
    {
    case boolType: return data->data.boolData ? 1. : 0.;
    case doubleType: return data->data.doubleData;
    case intType: return (double)data->data.intData;
    case uintType: return (double)data->data.uintData;
    case int64Type: return (double)data->data.int64Data;
    case uint64Type: return (double)data->data.uint64Data;
    case stringType: return ((const String*)(data + 1))->toDouble();
    default:
      return 0.;
    }
  }

  Variant& operator=(double other)
  {
    if(data->type != doubleType)
    {
      clear();
      data = &_data;
      _data.type = doubleType;
      _data.ref = 0;
    }
    _data.data.doubleData = other;
    return *this;
  }

  int toInt() const
  {
    switch(data->type)
    {
    case boolType: return data->data.boolData ? 1 : 0;
    case doubleType: return (int)data->data.doubleData;
    case intType: return data->data.intData;
    case uintType: return (int)data->data.uintData;
    case int64Type: return (int)data->data.int64Data;
    case uint64Type: return (int)data->data.uint64Data;
    case stringType: return ((const String*)(data + 1))->toInt();
    default:
      return 0;
    }
  }

  Variant& operator=(int other)
  {
    if(data->type != intType)
    {
      clear();
      data = &_data;
      _data.type = intType;
      _data.ref = 0;
    }
    _data.data.intData = other;
    return *this;
  }

  int toUInt() const
  {
    switch(data->type)
    {
    case boolType: return data->data.boolData ? 1 : 0;
    case doubleType: return (uint)data->data.doubleData;
    case intType: return (uint)data->data.intData;
    case uintType: return data->data.uintData;
    case int64Type: return (uint)data->data.int64Data;
    case uint64Type: return (uint)data->data.uint64Data;
    case stringType: return ((const String*)(data + 1))->toUInt();
    default:
      return 0;
    }
  }

  Variant& operator=(uint other)
  {
    if(data->type != uintType)
    {
      clear();
      data = &_data;
      _data.type = uintType;
      _data.ref = 0;
    }
    _data.data.uintData = other;
    return *this;
  }

  int64 toInt64() const
  {
    switch(data->type)
    {
    case boolType: return data->data.boolData ? 1 : 0;
    case doubleType: return (int64)data->data.doubleData;
    case intType: return (int64)data->data.intData;
    case uintType: return (int64)data->data.uintData;
    case int64Type: return data->data.int64Data;
    case uint64Type: return (int64)data->data.uint64Data;
    case stringType: return ((const String*)(data + 1))->toInt64();
    default:
      return 0;
    }
  }

  Variant& operator=(int64 other)
  {
    if(data->type != int64Type)
    {
      clear();
      data = &_data;
      _data.type = int64Type;
      _data.ref = 0;
    }
    _data.data.int64Data = other;
    return *this;
  }

  uint64 toUInt64() const
  {
    switch(data->type)
    {
    case boolType: return data->data.boolData ? 1 : 0;
    case doubleType: return (uint64)data->data.doubleData;
    case intType: return (uint64)data->data.intData;
    case uintType: return (uint64)data->data.uintData;
    case int64Type: return (uint64)data->data.int64Data;
    case uint64Type: return data->data.uint64Data;
    case stringType: return ((const String*)(data + 1))->toUInt64();
    default:
      return 0;
    }
  }

  Variant& operator=(uint64 other)
  {
    if(data->type != uint64Type)
    {
      clear();
      data = &_data;
      _data.type = uint64Type;
      _data.ref = 0;
    }
    _data.data.uint64Data = other;
    return *this;
  }

  const HashMap<String, Variant>& toMap() const
  {
    if(data->type == mapType)
      return *(const HashMap<String, Variant>*)(data + 1);
    static const HashMap<String, Variant> map;
    return map;
  }

  HashMap<String, Variant>& toMap()
  {
    if(data->type != mapType)
    {
      clear();
      data = (Data*)Memory::alloc(sizeof(Data) + sizeof(HashMap<String, Variant>));
      HashMap<String, Variant>* map = (HashMap<String, Variant>*)(data + 1);
      new (map) HashMap<String, Variant>;
      data->type = mapType;
      data->ref = 1;
      return *map;
    }
    else if(data->ref > 1)
    {
      Data* newData = (Data*)Memory::alloc(sizeof(Data) + sizeof(HashMap<String, Variant>));
      HashMap<String, Variant>* map = (HashMap<String, Variant>*)(data + 1);
      new (map) HashMap<String, Variant>(*(const HashMap<String, Variant>*)(data + 1));
      clear();
      data = newData;
      data->type = mapType;
      data->ref = 1;
      return *map;
    }
    return *(HashMap<String, Variant>*)(data + 1);
  }

  Variant& operator=(const HashMap<String, Variant>& other)
  {
    if(data->type != mapType || data->ref > 1)
    {
      clear();
      data = (Data*)Memory::alloc(sizeof(Data) + sizeof(HashMap<String, Variant>));
      HashMap<String, Variant>* map = (HashMap<String, Variant>*)(data + 1);
      new (map) HashMap<String, Variant>(other);
      data->type = mapType;
      data->ref = 1;
    }
    else
      *(HashMap<String, Variant>*)(data + 1) = other;
    return *this;
  }

  const List<Variant>& toList() const
  {
    if(data->type == listType)
      return *(const List<Variant>*)(data + 1);
    static const List<Variant> list;
    return list;
  }

  List<Variant>& toList()
  {
    if(data->type != listType)
    {
      clear();
      data = (Data*)Memory::alloc(sizeof(Data) + sizeof(List<Variant>));
      List<Variant>* list = (List<Variant>*)(data + 1);
      new (list) List<Variant>;
      data->type = listType;
      data->ref = 1;
      return *list;
    }
    else if(data->ref > 1)
    {
      Data* newData = (Data*)Memory::alloc(sizeof(Data) + sizeof(List< Variant>));
      List<Variant>* list = (List<Variant>*)(data + 1);
      new (list) List<Variant>(*(const List<Variant>*)(data + 1));
      clear();
      data = newData;
      data->type = listType;
      data->ref = 1;
      return *list;
    }
    return *(List<Variant>*)(data + 1);
  }

  Variant& operator=(const List<Variant>& other)
  {
    if(data->type != listType || data->ref > 1)
    {
      clear();
      data = (Data*)Memory::alloc(sizeof(Data) + sizeof(List<Variant>));
      List< Variant>* list = (List<Variant>*)(data + 1);
      new (list) List<Variant>(other);
      data->type = listType;
      data->ref = 1;
    }
    else
      *(List<Variant>*)(data + 1) = other;
    return *this;
  }

  String toString() const
  {
    if(data->type == stringType)
      return *(const String*)(data + 1);
    String string;
    switch(data->type)
    {
    case boolType: string = data->data.boolData ? String(_T("true")) : String(_T("false")); break;
    case doubleType: string.printf(_T("%f"), data->data.doubleData); break;
    case intType: string.printf(_T("%d"), data->data.intData); break;
    case uintType: string.printf(_T("%u"), data->data.uintData); break;
    case int64Type: string.printf(_T("%lld"), data->data.int64Data); break;
    case uint64Type: string.printf(_T("%llu"), data->data.uint64Data); break;
    default: break;
    }
    return string;
  }

  Variant& operator=(const String& other)
  {
    if(data->type != stringType || data->ref > 1)
    {
      clear();
      data = (Data*)Memory::alloc(sizeof(Data) + sizeof(String));
      String* string = (String*)(data + 1);
      new (string) String(other);
      data->type = stringType;
      data->ref = 1;
    }
    else
      *(String*)(data + 1) = other;
    return *this;
  }

  void swap(Variant& other)
  {
    Variant tmp = other;
    other = *this;
    *this = tmp;
  }

private:
  struct Data
  {
    Type type;
    union data
    {
      int intData;
      uint uintData;
      double doubleData;
      bool boolData;
      int64 int64Data;
      uint64 uint64Data;
    } data;
    volatile usize ref;
  };

  static struct NullData : public Data
  {
    NullData()
    {
      type = nullType;
      ref = 0;
    }
  } nullData;

  Data* data;
  Data _data;
};
