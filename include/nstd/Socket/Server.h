
#pragma once

#include <nstd/Event.h>

class Socket;

class Server
{
public:
  class Client : public Event::Source
  {
  public: // events
    void failed() {}
    void opened() {}
    void read() {}
    void wrote() {}
    void closed() {}

  public:
    bool write(const byte* data, usize size, usize* postponed = 0);
    bool read(byte* buffer, usize maxSize, usize& size);
    void suspend();
    void resume();
    void close();
  };

  class Listener : public Event::Source
  {
  public: // events
    void accepted() {}

  public:
    Client* accept(uint32* addr = 0, uint16* port = 0);
    void close();
  };

  class Timer : public Event::Source
  {
  public: // events
    void activated() {}

  public:
    void close();
  };

public:
  Server();
  ~Server();

  Listener* listen(uint16 port);
  Listener* listen(uint32 addr, uint16 port);
  Client* connect(uint32 addr, uint16 port);
  Client* pair(Socket& socket);
  Timer* createTimer(int64 interval);
  Client* accept(Listener& listener, uint32* addr = 0, uint16* port = 0);

  bool wait();
  bool interrupt();

  void setKeepAlive(bool enable = true);
  void setNoDelay(bool enable = true);
  void setSendBufferSize(int size);
  void setReceiveBufferSize(int size);

private:
  Server(const Server&);
  Server& operator=(const Server&);

private:
  class Private;
  Private* p;
};
