
#include <nstd/Debug.h>
#include <nstd/List.h>
#include <nstd/String.h>
#include <nstd/Math.h>

void testList()
{
  {
    // test list append
    List<String> myList;
    List<String> emptyList;
    myList.swap(emptyList);
    ASSERT(myList.isEmpty());
    myList.append(_T("string1"));
    myList.append(_T("string2"));
    myList.append(_T("string3"));
    ASSERT(myList.size() == 3);

    // test list copy constructor
    List<String> myList2(myList);
    ASSERT(myList2.size() == 3);
    ASSERT(*myList2.begin() == _T("string1"));
    ASSERT(*(++List<String>::Iterator(myList2.begin())) == _T("string2"));
    ASSERT(*(++myList2.begin()) == _T("string2"));
    ASSERT(myList2.back() == _T("string3"));

    // test list copy operator
    List<String> myList3;
    myList3 = myList;
    ASSERT(myList3.size() == 3);
    ASSERT(*myList3.begin() == _T("string1"));
    ASSERT(*(++List<String>::Iterator(myList3.begin())) == _T("string2"));
    ASSERT(myList3.back() == _T("string3"));

    // test list find
    ASSERT(myList.find(_T("string2")) != myList.end());
    ASSERT(myList.find(_T("string4")) == myList.end());
    myList.remove(_T("string2"));
    ASSERT(myList.size() == 2);
    ASSERT(myList.find(_T("string2")) == myList.end());

    // test list iterator
    List<String>::Iterator it = myList.begin();
    ASSERT(*it == _T("string1"));
    ASSERT(*(++it) == _T("string3"));
    *it = _T("abbba");
    ASSERT(*it == _T("abbba"));

    // test prepend
    myList.prepend(_T("string7"));
    ASSERT(*myList.begin() == _T("string7"));

    // test list clear
    myList.clear();
    ASSERT(myList.size() == 0);
    ASSERT(myList.isEmpty());

    // test front and back
    myList.clear();
    myList.append(_T("1"));
    myList.append(_T("2"));
    ASSERT(myList.front() == _T("1"));
    ASSERT(myList.back() == _T("2"));
    myList.append(_T("3"));
    ASSERT(myList.front() == _T("1"));
    ASSERT(myList.back() == _T("3"));
    myList.prepend(_T("0"));
    ASSERT(myList.front() == _T("0"));
    ASSERT(myList.back() == _T("3"));

    // test remove front and back
    myList.clear();
    myList.append(_T("1"));
    myList.append(_T("2"));
    myList.append(_T("3"));
    myList.removeFront();
    ASSERT(myList.size() == 2);
    ASSERT(myList.front() == _T("2"));
    myList.removeBack();
    ASSERT(myList.size() == 1);
    ASSERT(myList.front() == _T("2"));
    ASSERT(myList.back() == _T("2"));
  }

  // int sort
  {
    List<int> myList;
    for(int i = 0; i < 100; ++i)
      myList.append(Math::random() % 90);
    myList.sort();
    int current = 0;
    for(List<int>::Iterator i = myList.begin(), end = myList.end(); i != end; ++i)
    {
      ASSERT(*i >= current);
      current = *i;
    }
  }

  // string sort
  {
    List<String> myList;
    String str;
    for(int i = 0; i < 100; ++i)
    {
      str.printf(_T("abc%d"), (int)(Math::random() % 90));
      myList.append(str);
    }
    myList.sort();
    String current(_T("abc0"));
    for(List<String>::Iterator i = myList.begin(), end = myList.end(); i != end; ++i)
    {
      ASSERT(*i >= current);
      current = *i;
    }
  }
}
