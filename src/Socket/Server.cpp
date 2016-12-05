
#include <nstd/Socket/Socket.h>
#include <nstd/Socket/Server.h>
#include <nstd/Pool.h>
#include <nstd/Error.h>
#include <nstd/Buffer.h>
#include <nstd/List.h>
#include <nstd/MultiMap.h>
#include <nstd/Time.h>

struct Server::Handle
{
  enum Type
  {
    clientType,
    timerType,
    listenerType,
  };

  enum State
  {
    connectingState,
    connectedState,
    suspendedState,
    closingState,
    closedState,
  };

  Type type;
  State state;
  void* userData;
};

class Server::Private
{
public:
  class HandleSocket : public Socket
  {
  public:
    Handle* handle;
  };

  struct Listener : public Handle
  {
    HandleSocket socket;
  };

  struct Client : public Handle
  {
    HandleSocket socket;
    Buffer sendBuffer;
  };

  struct Timer : public Handle
  {
    int64 executionTime;
    int64 interval;
  };

private:
  Server& o;
  Pool<Listener> listeners;
  Pool<Client> clients;
  Pool<Timer> timers;
  MultiMap<int64, Timer*> queuedTimers;
  Socket::Poll sockets;
  List<Handle*> closingHandles;

  bool keepAlive;
  bool noDelay;
  int sendBufferSize;
  int receiveBufferSize;

  bool interrupted;

public:
  Private(Server& o) : o(o), keepAlive(false), noDelay(false), sendBufferSize(0), receiveBufferSize(0), interrupted(false)
  {
    queuedTimers.insert(0, 0); // add default timeout timer
  }

  Handle* listen(uint32 addr, uint16 port, void* userData)
  {
    Socket socket;
    if(!socket.open() ||
      !socket.setReuseAddress() ||
      !socket.bind(addr, port) ||
      !socket.listen())
      return 0;
    Listener& listener = listeners.append();
    listener.type = Handle::listenerType;
    listener.state = Handle::connectedState;
    listener.socket.swap(socket);
    listener.socket.handle = &listener;
    listener.userData = userData;
    sockets.set(listener.socket, Socket::Poll::acceptFlag);
    return &listener;
  }

  Handle* accept(Handle& handle, void* userData, uint32* addr, uint16* port)
  {
    Listener& listener = (Listener&)handle;
    Socket socket;
    uint32 addr2;
    uint16 port2;
    if(!listener.socket.accept(socket, *(addr ? addr : &addr2), *(port ? port : &port2)) ||
      !socket.setNonBlocking() ||
      (keepAlive && !socket.setKeepAlive()) ||
      (noDelay && !socket.setNoDelay()) ||
      (sendBufferSize > 0 && !socket.setSendBufferSize(sendBufferSize)) ||
      (receiveBufferSize > 0 && !socket.setReceiveBufferSize(receiveBufferSize)))
      return 0;
    Client& client = clients.append();
    client.type = Handle::clientType;
    client.state = Handle::connectedState;
    client.socket.swap(socket);
    client.socket.handle = &client;
    client.userData = userData;
    sockets.set(client.socket, Socket::Poll::readFlag);
    return &client;
  }

  Handle* connect(uint32 addr, uint16 port, void* userData)
  {
    Socket socket;
    if(!socket.open() ||
      !socket.setNonBlocking() ||
      !socket.connect(addr, port))
      return 0;
    Client& client = clients.append();
    client.type = Handle::clientType;
    client.state = Handle::connectingState;
    client.socket.swap(socket);
    client.socket.handle = &client;
    client.userData = userData;
    sockets.set(client.socket, Socket::Poll::connectFlag);
    return &client;
  }

  Handle* pair(Socket& otherSocket, void* userData)
  {
    Socket socket;
    if(!socket.pair(otherSocket) ||
      !socket.setNonBlocking() ||
      (keepAlive && !socket.setKeepAlive()) ||
      (noDelay && !socket.setNoDelay()) ||
      (sendBufferSize > 0 && !socket.setSendBufferSize(sendBufferSize)) ||
      (receiveBufferSize > 0 && !socket.setReceiveBufferSize(receiveBufferSize)))
    {
      otherSocket.close();
      return 0;
    }
    Client& client = clients.append();
    client.type = Handle::clientType;
    client.state = Handle::connectedState;
    client.socket.swap(socket);
    client.socket.handle = &client;
    client.userData = userData;
    sockets.set(client.socket, Socket::Poll::readFlag);
    return &client;
  }

  Handle* createTimer(int64 interval, void* userData)
  {
    int64 executionTime = Time::ticks() + interval;
    Timer& timer = timers.append();
    timer.type = Handle::timerType;
    timer.state = Handle::connectedState;
    timer.userData = userData;
    timer.executionTime = executionTime;
    timer.interval = interval;
    queuedTimers.insert(executionTime, &timer);
    return &timer;
  }

  bool write(Handle& handle, const byte* data, usize size, usize* postponed)
  {
    if(handle.type != Handle::clientType)
      return false;
    Client& client = (Client&)handle;
    if(client.state != Client::connectedState && client.state != Client::suspendedState)
      return false;
    if(client.sendBuffer.isEmpty())
    {
      ssize sent = client.socket.send(data, size);
      switch(sent)
      {
      case -1:
        if(Socket::getLastError() == 0) // EWOULDBLOCK
        {
          sent = 0;
          break;
        }
        // no break
      case 0:
        client.state = Client::closingState;
        closingHandles.append(&client);
        return false;
      default:
        break;
      }
      if((usize)sent >= size)
      {
        if(postponed)
          *postponed = 0;
        return true;
      }
      client.sendBuffer.append(data + sent, size - sent);
      sockets.set(client.socket, Socket::Poll::writeFlag);
    }
    else
      client.sendBuffer.append(data, size);
    if(postponed)
      *postponed = client.sendBuffer.size();
    return true;
  }

  bool read(Handle& handle, byte* buffer, usize maxSize, usize& size)
  {
    if(handle.type != Handle::clientType)
      return false;
    Client& client = (Client&)handle;
    if(client.state != Client::connectedState && client.state != Client::suspendedState)
      return false;
    ssize received = client.socket.recv(buffer, maxSize);
    switch(received)
    {
    case -1:
      if(Socket::getLastError() == 0) // EWOULDBLOCK
        return false;
      // no break
    case 0:
      client.state = Client::closingState;
      closingHandles.append(&client);
      return false;
    default:
      break;
    }
    size = (usize)received;
    return true;
  }

  void close(Handle& handle)
  {
    closingHandles.remove(&handle);
    switch(handle.type)
    {
    case Handle::clientType:
      {
        Client& client = (Client&)handle;
        sockets.remove(client.socket);
        clients.remove(client);
      }
      break;
    case Handle::timerType:
      {
        Timer& timer = (Timer&)handle;
        if(timer.state == Handle::connectedState)
          for(MultiMap<int64, Timer*>::Iterator i = queuedTimers.find(timer.executionTime), end = queuedTimers.end(); i != end; ++i)
          {
            if(*i == &timer)
            {
              queuedTimers.remove(i);
              break;
            }
            if(i.key() != timer.executionTime)
              break;
          }
        timers.remove(timer);
      }
      break;
    case Handle::listenerType:
      {
        Listener& listener = (Listener&)handle;
        sockets.remove(listener.socket);
        listeners.remove(listener);
      }
      break;
    }
  }

  bool wait()
  {
  start:
    while(!closingHandles.isEmpty())
    {
      Handle& handle = *closingHandles.front();
      closingHandles.removeFront();
      switch(handle.type)
      {
      case Handle::clientType:
        if(handle.state == Handle::closingState)
        {
          Client& client = (Client&)handle;
          client.state = Client::closedState;
          sockets.remove(client.socket);
          o.emit<Server, Handle&, void*>(&Server::clientClosed, client, client.userData);
          goto start;
        }
        break;
      case Handle::timerType:
        if(handle.state == Handle::suspendedState)
        {
          Timer& timer = (Timer&)handle;
          timer.executionTime += timer.interval;
          queuedTimers.insert(timer.executionTime, &timer);
          timer.state = Handle::connectedState;
        }
        break;
      default:
        break;
      }
    }

    for(Socket::Poll::Event pollEvent;;)
    {
      int64 now = Time::ticks();
      int64 timeout = queuedTimers.begin().key() - now;
      for(; timeout <= 0; timeout = queuedTimers.begin().key() - now)
      {
        Timer* timer = queuedTimers.front();
        queuedTimers.removeFront();
        if(timer) // user timer
        {
          timer->state = Handle::suspendedState;
          closingHandles.append(timer);
          o.emit<Server, Handle&, void*>(&Server::timerActivated, *timer, timer->userData);
          goto start;
        }
        else
          queuedTimers.insert(now + 300 * 1000, 0); // keep "default timeout" timer
      }

      if(!sockets.poll(pollEvent, timeout))
        break;

      if(!pollEvent.flags)
      {
        if(interrupted)
        {
          interrupted = false;
          return true;
        }
        continue; // timeout
      }
      Handle* handle = ((HandleSocket*)pollEvent.socket)->handle;
      if(pollEvent.flags & Socket::Poll::readFlag)
      {
        o.emit<Server, Handle&, void*>(&Server::clientRead, *handle, handle->userData);
        goto start;
      }
      else if(pollEvent.flags & Socket::Poll::writeFlag)
      {
        Client& client = *(Client*)handle;
        if(!client.sendBuffer.isEmpty())
        {
          ssize sent = client.socket.send(client.sendBuffer, client.sendBuffer.size());
          switch(sent)
          {
          case -1:
            if(Socket::getLastError() == 0) // EWOULDBLOCK
              continue;
            // no break
          case 0:
            client.sendBuffer.free();
            client.state = Client::closedState;
            sockets.remove(client.socket);
            o.emit<Server, Handle&, void*>(&Server::clientClosed, *handle, handle->userData);
            goto start;
          default:
            break;
          }
          client.sendBuffer.removeFront((usize)sent);
        }
        if(client.sendBuffer.isEmpty())
        {
          client.sendBuffer.free();
          if(client.state != Client::suspendedState)
            sockets.set(client.socket, Socket::Poll::readFlag);
          else
            sockets.remove(client.socket);
          o.emit<Server, Handle&, void*>(&Server::clientWrote, *handle, handle->userData);
          goto start;
        }
        continue;
      }
      else if(pollEvent.flags & Socket::Poll::acceptFlag)
      {
        o.emit<Server, Handle&, void*>(&Server::clientAccpeted, *handle, handle->userData);
        goto start;
      }
      else if(pollEvent.flags & Socket::Poll::connectFlag)
      {
        Client& client = *(Client*)handle;
        Socket& socket = client.socket;
        int error = socket.getAndResetErrorStatus();
        if(error)
        {
          Error::setLastError((uint)error);
          client.state = Client::closedState;
          sockets.remove(client.socket);
          o.emit<Server, Handle&, void*>(&Server::clientFailed, *handle, handle->userData);
          goto start;
        }
        else
        {
          if((keepAlive && !socket.setKeepAlive()) ||
            (noDelay && !socket.setNoDelay()) ||
            (sendBufferSize > 0 && !socket.setSendBufferSize(sendBufferSize)) ||
            (receiveBufferSize > 0 && !socket.setReceiveBufferSize(receiveBufferSize)))
          {
            client.state = Client::closedState;
            sockets.remove(client.socket);
            o.emit<Server, Handle&, void*>(&Server::clientFailed, *handle, handle->userData);
            goto start;
          }
          else
          {
            client.state = Client::connectedState;
            sockets.set(*pollEvent.socket, Socket::Poll::readFlag);
            o.emit<Server, Handle&, void*>(&Server::clientOpened, *handle, handle->userData);
            goto start;
          }
        }
      }
    }
    return false;
  }

  bool interrupt()
  {
    interrupted = true;
    return sockets.interrupt();
  }

  void suspend(Handle& handle)
  {
    if(handle.type != Handle::clientType)
      return;
    Client& client = (Client&)handle;
    if(client.state != Client::connectedState)
      return;
    if(client.sendBuffer.isEmpty())
      sockets.remove(client.socket);
    client.state = Client::suspendedState;
  }

  void resume(Handle& handle)
  {
    if(handle.type != Handle::clientType)
      return;
    Client& client = (Client&)handle;
    if(client.state != Client::suspendedState)
      return;
    if(client.sendBuffer.isEmpty())
      sockets.set(client.socket, Socket::Poll::readFlag);
    client.state = Client::connectedState;
  }

  void setKeepAlive(bool enable) {keepAlive = enable;}
  void setNoDelay(bool enable) {noDelay = enable;}
  void setSendBufferSize(int size) {sendBufferSize = size;}
  void setReceiveBufferSize(int size) {receiveBufferSize = size;}
};

Server::Server() : p(new Private(*this)) {}
Server::~Server() {delete p;}
Server::Handle* Server::listen(uint16 port, void* userData) {return p->listen(Socket::anyAddr, port, userData);}
Server::Handle* Server::listen(uint32 addr, uint16 port, void* userData) {return p->listen(addr, port, userData);}
Server::Handle* Server::connect(uint32 addr, uint16 port, void* userData) {return p->connect(addr, port, userData);}
Server::Handle* Server::pair(Socket& socket, void* userData) {return p->pair(socket, userData);}
Server::Handle* Server::createTimer(int64 interval, void* userData) {return p->createTimer(interval, userData);}
Server::Handle* Server::accept(Handle& handle, void* userData, uint32* addr, uint16* port) {return p->accept(handle, userData, addr, port);}
void Server::setUserData(Handle& handle, void* userData) {handle.userData = userData;}
void* Server::getUserData(Handle& handle) {return handle.userData;}
bool Server::write(Handle& handle, const byte* data, usize size, usize* postponed) {return p->write(handle, data, size, postponed);}
bool Server::read(Handle& handle, byte* buffer, usize maxSize, usize& size) {return p->read(handle, buffer, maxSize, size);}
void Server::close(Handle& handle) {return p->close(handle);}
bool Server::wait() {return p->wait();}
bool Server::interrupt() {return p->interrupt();}
void Server::suspend(Handle& handle) {return p->suspend(handle);}
void Server::resume(Handle& handle) {return p->resume(handle);}
void Server::setKeepAlive(bool enable) {return p->setKeepAlive(enable);}
void Server::setNoDelay(bool enable) {return p->setNoDelay(enable);}
void Server::setSendBufferSize(int size) {return p->setSendBufferSize(size);}
void Server::setReceiveBufferSize(int size) {return p->setReceiveBufferSize(size);}
