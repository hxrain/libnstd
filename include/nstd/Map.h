
#pragma once

#include <nstd/Debug.h>

template<typename T, typename V> class Map
{
private:
  struct Item;
public:
  class Iterator
  {
  public:
    Iterator() : item(0) {}
    const T& key() const {return item->key;}
    V& operator*() {return item->value;}
    const V& operator*() const {return item->value;}
    const V* operator->() const {return &item->value;}
    const Iterator& operator++() {item = item->next; return *this;}
    const Iterator& operator--() {item = item->prev; return *this;}
    bool operator==(const Iterator& other) const {return item == other.item;}
    bool operator!=(const Iterator& other) const {return item != other.item;}

  private:
    Item* item;
    
    Iterator(Item* item) : item(item) {}

    friend class Map;
  };
  
public:
  Map() : _end(&endItem), _begin(&endItem), _size(0), root(0)
  {
    endItem.parent = 0;
    endItem.prev = 0;
    endItem.next = 0;
  }

  const Iterator& begin() const {return _begin;}
  const Iterator& end() const {return _end;}

  const T& front() const {return _begin.item->value;}
  const T& back() const {return _end.item->prev->value;}

  T& front() {return _begin.item->value;}
  T& back() {return _end.item->prev->value;}

  //Iterator removeFront() {return remove(_begin);}
  //Iterator removeBack() {return remove(_end.item->prev);}

  size_t size() const {return _size;}
  bool_t isEmpty() const {return !root;}

  Iterator find(const T& key) const
  {
    for(Item* item = root; item; )
    {
      if(key > item->key)
      {
        item = item->right;
        continue;
      }
      else if(key < item->key)
      {
        item = item->left;
        continue;
      }
      else
        return item;
    }
    return _end;
  }

  Iterator insert(const Iterator& position, const T& key, const V& value)
  {
    Item* insertPos = position.item;
    if(insertPos == &endItem)
    {
      Item* prev = insertPos->prev;
      if(prev && key > prev->key)
        return insert(&prev->right, prev, key, value);
    }
    else
    {
      if(key < insertPos->key)
      {
        Item* prev = insertPos->prev;
        if(!prev || key > prev->key)
          return insert(&insertPos->left, insertPos, key, value);
      }
      else if(key > insertPos->key)
      {
        Item* next = insertPos->next;
        if(next == &endItem || key < next->key)
          return insert(&insertPos->right, insertPos, key, value);
      }
      else
      {
        insertPos->value = value;
        return insertPos;
      }
    }
    return insert(&root, 0, key, value);
  }
  
  Iterator insert(const T& key, const V& value)
  {
    return insert(&root, 0, key, value);
  }

  void_t remove(const T& key)
  {
    Iterator it = find(key);
    if(it != _end)
      remove(it);
  }

  Iterator remove(const Iterator& it)
  {
    Item* item = it.item;
    Item* origParent;
    Item* parent = origParent = item->parent;
    Item** cell = parent ? (item == parent->left ? &parent->left : &parent->right) : &root;
    ASSERT(*cell == item);
    Item* left = item->left;
    Item* right = item->right;

    Item* prev = item->prev;
    Item* next = item->next; // todo: use freeItem stuff

    if(!item->prev)
      (_begin.item = item->next)->prev = 0;
    else
      (item->prev->next = item->next)->prev = item->prev;
    --_size;
    delete item;

    if(!left && !right)
    {
      *cell = 0;
      goto rebalParentUpwards;
    }

    if(!left)
    {
      *cell = right;
      right->parent = parent;
      goto rebalParentUpwards;
    }

    if(!right)
    {
      *cell = left;
      left->parent = parent;
      goto rebalParentUpwards;
    }

    if(left->height < right->height)
    {
      ASSERT(!next->left);
      Item* nextParent = next->parent;
      if(nextParent == item)
      {
        ASSERT(next == right);
        *cell = next;
        next->parent = parent;

        next->left = left;
        left->parent = next;
        parent = next;
        goto rebalParent;
      }

      // unlink next
      ASSERT(next == nextParent->left);
      Item* nextRight = next->right;
      nextParent->left = nextRight;
      if(nextRight)
        nextRight->parent = nextParent;
      
      // put next in cell
      *cell = next;
      next->parent = parent;

      // link left to next left
      next->left = left;
      left->parent = next;

      // link right to next right
      next->right = right;
      right->parent = next;

      // rebal old next->parent
      parent = nextParent;
      ASSERT(parent);
      goto rebalParent;
    }
    else
    {
      ASSERT(!prev->right);
      Item* prevParent = prev->parent;
      if(prevParent == item)
      {
        ASSERT(prev == left);
        *cell = prev;
        prev->parent = parent;

        prev->right = right;
        right->parent = prev;
        parent = prev;
        goto rebalParent;
      }

      // unlink prev
      ASSERT(prev == prevParent->right);
      Item* prevLeft = prev->left;
      prevParent->right = prevLeft;
      if(prevLeft)
        prevLeft->parent = prevParent;

      // put prev in cell
      *cell = prev;
      prev->parent = parent;

      // link right to prev right
      prev->right = right;
      right->parent = prev;

      // link left to prev left
      prev->left = left;
      left->parent = prev;

      // rebal old prev->parent
      parent = prevParent;
      ASSERT(parent);
      goto rebalParent;
    }

    // rebal parent upwards
  rebalParent:
    do
    {
      parent->updateHeightAndSlope();
      Item* parentParent = parent->parent;
      rebal(parent);
      parent = parentParent;
    } while(parent != origParent);
  rebalParentUpwards:
    while(parent)
    {
      size_t oldHeight = parent->height;
      parent->updateHeightAndSlope();
      if(parent->height == oldHeight)
        break;
      Item* parentParent = parent->parent;
      rebal(parent);
      parent = parentParent;
    }
    return next;
  }

private:
  struct Item
  {
    const T key;
    V value;
    Item* parent;
    Item* left;
    Item* right;
    size_t height;
    ssize_t slope;
    Item* next;
    Item* prev;
    
    Item() : key(), value() {}
    
    Item(Item* parent, const T& key, const V& value) : key(key), value(value), parent(parent), left(0), right(0), height(1), slope(0) {}
    
    void_t updateHeightAndSlope()
    {
      ASSERT(!parent || this == parent->left || this == parent->right);
      size_t leftHeight = left ? left->height : 0;
      size_t rightHeight = right ? right->height : 0;
      slope = leftHeight - rightHeight;
      height = (leftHeight > rightHeight ? leftHeight : rightHeight) + 1;
    }
  };
  
private:
  Iterator _end;
  Iterator _begin;
  Item* root;
  size_t _size;
  Item endItem;

private:
  Iterator insert(Item** cell, Item* parent, const T& key, const V& value)
  {
  begin:
    ASSERT(!parent || cell == &parent->left || cell == &parent->right);
    Item* position = *cell;
    if(!position)
    {
      Item* item = new Item(parent, key, value);
      *cell = item;
      ++_size;
      if(!parent)
      { // first item
        ASSERT(_begin.item == &endItem);
        item->prev = 0;
        item->next = _begin.item;
        _begin.item = item;
        endItem.prev = item;
      }
      else
      {
        Item* insertPos = cell == &parent->right ? 
          parent->next : // insert after parent
          parent; // insert before parent
        if((item->prev = insertPos->prev))
          insertPos->prev->next = item;
        else
          _begin.item = item;
        item->next = insertPos;
        insertPos->prev = item;

        do
        {
          size_t oldHeight = parent->height;
          parent->updateHeightAndSlope();
          if(parent->height == oldHeight)
            break;
          Item* parentParent = parent->parent;
          rebal(parent);
          parent = parentParent;
        } while(parent);
      }
      return item;
    }
    else
    {
      if(key > position->key)
      {
        cell = &position->right;
        parent = position;
        goto begin;
      }
      else if(key < position->key)
      {
        cell = &position->left;
        parent = position;
        goto begin;
      }
      else
      {
        position->value = value;
        return position;
      }
    }
  }
  
  void_t rebal(Item* item)
  {
    if(item->slope > 1)
    {
      ASSERT(item->slope == 2);
      Item* parent = item->parent;
      Item*& cell = parent ? (parent->left == item ? parent->left : parent->right) : root;
      ASSERT(cell == item);
      shiftr(cell);
      ASSERT((cell->slope < 0 ? -cell->slope : cell->slope) <= 1);
    }
    else if(item->slope < -1)
    {
      ASSERT(item->slope == -2);
      Item* parent = item->parent;
      Item*& cell = parent ? (parent->left == item ? parent->left : parent->right) : root;
      ASSERT(cell == item);
      shiftl(cell);
      ASSERT((cell->slope < 0 ? -cell->slope : cell->slope) <= 1);
    }
  }
  
  static void_t rotr(Item*& cell)
  {
    Item* oldTop = cell;
    Item* result = oldTop->left;
    Item* tmp = result->right;
    result->parent = oldTop->parent;
    result->right = oldTop;
    if((oldTop->left = tmp))
      tmp->parent = oldTop;
    oldTop->parent = result;
    cell = result;
    oldTop->updateHeightAndSlope();
    result->updateHeightAndSlope();
  }
  
  static void_t rotl(Item*& cell)
  {
    Item* oldTop = cell;
    Item* result = oldTop->right;
    Item* tmp = result->left;
    result->parent = oldTop->parent;
    result->left = oldTop;
    if((oldTop->right = tmp))
      tmp->parent = oldTop;
    oldTop->parent = result;
    cell = result;
    oldTop->updateHeightAndSlope();
    result->updateHeightAndSlope();
  }
  
  static void_t shiftr(Item*& cell)
  {
    Item* oldTop = cell;
    if(oldTop->left->slope == -1)
      rotl(oldTop->left);
    rotr(cell);
  }

  static void_t shiftl(Item*& cell)
  {
    Item* oldTop = cell;
    if(oldTop->right->slope == 1)
      rotr(oldTop->right);
    rotl(cell);
  }

};