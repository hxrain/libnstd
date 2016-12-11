
#include <nstd/Debug.h>
#include <nstd/Pool.h>
#include <nstd/Socket/Server.h>
#include <nstd/Socket/Socket.h>

void testServer()
{
  // test timer
  {
    Server server;
    Server::Timer* timer = server.createTimer(10);
    ASSERT(timer);
    class TimerListener : public Event::Listener
    {
    public:
      TimerListener(Server* server) : server(server), activations(0) {}
      void handleTimerActivated()
      {
        if(++activations == 3)
          server->interrupt();
      }
      Server* server;
      int activations;
    } timerListener(&server);
    Event::connect(timer, &Server::Timer::activated, &timerListener, &TimerListener::handleTimerActivated);
    ASSERT(server.wait());
    ASSERT(timerListener.activations == 3);
    timer->close();
  }

  // test connect fail
  {
    Server server;
    class ClientListener : public Event::Listener
    {
    public:
      ClientListener(Server* server) : activations(0), server(server) {}
      void handleClientFailed()
      {
        ++activations;
        server->interrupt();
      }
      int activations;
      Server* server;
    } clientListener(&server);
    Server::Client* client = server.connect(Socket::loopbackAddr, 7266);
    ASSERT(client);
    Event::connect(client, &Server::Client::failed, &clientListener, &ClientListener::handleClientFailed);
    ASSERT(server.wait());
    ASSERT(clientListener.activations == 1);
  }

  // test listen, connect, read, write and close
  {
    Server server;
    class ServerListener : public Event::Listener
    {
    public:
      ServerListener(Server& server) : server(server), state(connecting1) {}
      void handleListenerAccepted()
      {
        ASSERT(state == connecting1 || state == connecting2);
        client2 = listener->accept();
        ASSERT(client2);
        Event::connect(client2, &Server::Client::read, this, &ServerListener::handleClient2Read);
        Event::connect(client2, &Server::Client::closed, this, &ServerListener::handleClient2Closed);
        if(state == connecting1)
          state = connecting2;
        else
          state = sending1;
      }
      void handleClient1Opened()
      {
        ASSERT(state == connecting1 || state == connecting2);
        ASSERT(client1->write(testData, sizeof(testData)));
        if(state == connecting1)
          state = connecting2;
        else
          state = sending1;
      }
      void handleClient2Read()
      {
        ASSERT(state == sending1 || state == closing1);
        if(state == sending1)
        {
          usize size;
          ASSERT(client2->read(receiveBuffer, sizeof(receiveBuffer), size));
          ASSERT(size == sizeof(testData));
          ASSERT(client2->write(testData, sizeof(testData)));
          state = sending2;
        }
        else
        {
          usize size;
          ASSERT(!client2->read(receiveBuffer, sizeof(receiveBuffer), size));
          state = closing2;
        }
      }
      void handleClient1Read()
      {
        ASSERT(state == sending2);
        usize size;
        ASSERT(client1->read(receiveBuffer, sizeof(receiveBuffer), size));
        ASSERT(size == sizeof(testData));
        client1->close();
        state = closing1;
      }
      void handleClient2Closed()
      {
        ASSERT(state == closing2);
        server.interrupt();
        state = finished;
      }
      Server& server;
      enum State
      {
        connecting1,
        connecting2,
        sending1,
        sending2,
        closing1,
        closing2,
        finished,
      } state;
      byte testData[100];
      byte receiveBuffer[231];
      Server::Listener* listener;
      Server::Client* client1;
      Server::Client* client2;
    } serverListener(server);
    serverListener.listener = server.listen(7266);
    ASSERT(serverListener.listener);
    Event::connect(serverListener.listener, &Server::Listener::accepted, &serverListener, &ServerListener::handleListenerAccepted);
    serverListener.client1 = server.connect(Socket::loopbackAddr, 7266);
    ASSERT(serverListener.client1);
    Event::connect(serverListener.client1, &Server::Client::opened, &serverListener, &ServerListener::handleClient1Opened);
    Event::connect(serverListener.client1, &Server::Client::read, &serverListener, &ServerListener::handleClient1Read);
    ASSERT(server.wait());
    ASSERT(serverListener.state == ServerListener::finished);
  }
}
