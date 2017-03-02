
#include <nstd/Debug.h>
#include <nstd/Socket/Socket.h>

void testSocket()
{
  // test listening twice on a udp port
  {
    byte testData[123] = {1, 2, 3};
    byte rcvData[123];
    Socket listener1;
    Socket listener2;
    Socket sender;
    ASSERT(listener1.open(Socket::udpProtocol));
    ASSERT(listener2.open(Socket::udpProtocol));
    ASSERT(sender.open(Socket::udpProtocol));
    ASSERT(listener1.setReusePort());
    ASSERT(listener2.setReusePort());
    ASSERT(listener1.bind(Socket::anyAddr, 6212));
    ASSERT(listener2.bind(Socket::anyAddr, 6212));
    ASSERT(sender.bind(Socket::anyAddr, 0));
    ASSERT(sender.setBroadcast());
    ASSERT(sender.sendTo(testData, sizeof(testData), Socket::broadcastAddr, 6212) == sizeof(testData));
    uint32 ip;
    uint16 port;
    ASSERT(listener1.recvFrom(rcvData, sizeof(rcvData), ip, port) == sizeof(rcvData));
    ASSERT(Memory::compare(rcvData, testData, sizeof(rcvData)) == 0);
    ASSERT(listener2.recvFrom(rcvData, sizeof(rcvData), ip, port) == sizeof(rcvData));
    ASSERT(Memory::compare(rcvData, testData, sizeof(rcvData)) == 0);
  }

}
