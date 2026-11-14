#include "emscriptenSocket.h"

#include "tnlLog.h"

#include <emscripten.h>
#include <emscripten/websocket.h>

#include <cstdint>
#include <cstring>
#include <deque>
#include <map>
#include <string>
#include <vector>

using namespace TNL;

namespace
{

const uint8_t BIND = 1;
const uint8_t SEND = 2;
const uint8_t RECV = 3;
const uint8_t ALLOW = 4;
const S32 MAX_PAYLOAD = 8192;

struct Packet
{
   U32 ip;
   U16 port;
   std::vector<U8> data;
};

struct RelaySock
{
   U32 id;
   bool bound;
   std::deque<Packet> inbox;
   std::deque<std::vector<U8>> waiting;
};

const char *DEFAULT_RELAY = "wss://bitfighter-web-relay.spacewalkpublishing.us";
const double RELAY_GIVE_UP_MS = 90000.0;
const double RELAY_RETRY_MS = 2000.0;

std::string gUrl;
bool gUrlReady = false;
EMSCRIPTEN_WEBSOCKET_T gWs = 0;
bool gOpen = false;
bool gRetryScheduled = false;
bool gGaveUp = false;
double gFirstAttempt = 0;
double gOpenedAt = 0;
int gGeneration = 0;
std::map<U32, RelaySock> gSocks;
std::deque<std::vector<U8>> gEarly;
U32 gNextId = 1;

void writeU32(U8 *p, U32 v)
{
   p[0] = U8(v >> 24);
   p[1] = U8(v >> 16);
   p[2] = U8(v >> 8);
   p[3] = U8(v);
}

void writeU16(U8 *p, U16 v)
{
   p[0] = U8(v >> 8);
   p[1] = U8(v);
}

U32 readU32(const U8 *p)
{
   return (U32(p[0]) << 24) | (U32(p[1]) << 16) | (U32(p[2]) << 8) | U32(p[3]);
}

U16 readU16(const U8 *p)
{
   return U16((U16(p[0]) << 8) | U16(p[1]));
}

void sendFrame(const std::vector<U8> &frame)
{
   if(!gOpen)
   {
      gEarly.push_back(frame);
      return;
   }
   emscripten_websocket_send_binary(gWs, (void *)frame.data(), frame.size());
}

void flushEarly()
{
   while(!gEarly.empty())
   {
      std::vector<U8> frame = gEarly.front();
      gEarly.pop_front();
      emscripten_websocket_send_binary(gWs, frame.data(), frame.size());
   }
}

void handleFrame(const U8 *data, U32 len)
{
   if(len < 1)
      return;
   if(data[0] == BIND && len >= 7)
   {
      U32 id = readU32(data + 1);
      U16 port = readU16(data + 5);
      std::map<U32, RelaySock>::iterator it = gSocks.find(id);
      if(it == gSocks.end())
         return;
      it->second.bound = true;
      logprintf(LogConsumer::LogUDP, "Relay bound socket %u on UDP port %u", id, port);
      while(!it->second.waiting.empty())
      {
         sendFrame(it->second.waiting.front());
         it->second.waiting.pop_front();
      }
   }
   else if(data[0] == RECV && len >= 13)
   {
      U32 id = readU32(data + 1);
      U32 ip = readU32(data + 5);
      U16 port = readU16(data + 9);
      U16 payload = readU16(data + 11);
      if(U32(13 + payload) > len)
         return;
      std::map<U32, RelaySock>::iterator it = gSocks.find(id);
      if(it == gSocks.end())
         return;
      Packet packet;
      packet.ip = ip;
      packet.port = port;
      packet.data.assign(data + 13, data + 13 + payload);
      it->second.inbox.push_back(packet);
   }
}

void connectNow();

void retryCb(void *)
{
   gRetryScheduled = false;
   if(gOpen || gUrl.empty())
      return;
   connectNow();
}

void scheduleRetry()
{
   if(gRetryScheduled || gUrl.empty())
      return;
   double elapsed = emscripten_get_now() - gFirstAttempt;
   if(elapsed > RELAY_GIVE_UP_MS)
   {
      gGaveUp = true;
      logprintf(LogConsumer::LogError, "UDP relay did not connect within 90s (%s). A free host can take about a minute to wake; reload to try again.", gUrl.c_str());
      return;
   }
   gRetryScheduled = true;
   emscripten_set_timeout(retryCb, RELAY_RETRY_MS, 0);
}

EM_BOOL onOpen(int, const EmscriptenWebSocketOpenEvent *, void *userData)
{
   if((int)(intptr_t)userData != gGeneration)
      return EM_TRUE;
   gOpen = true;
   gGaveUp = false;
   // A later drop gets its own 90s of retries. The cold-start clock is the
   // stretch before the first successful open.
   gFirstAttempt = emscripten_get_now();
   gOpenedAt = gFirstAttempt;
   logprintf(LogConsumer::LogUDP, "UDP relay websocket open: %s", gUrl.c_str());
   flushEarly();
   return EM_TRUE;
}

EM_BOOL onClose(int, const EmscriptenWebSocketCloseEvent *, void *userData)
{
   if((int)(intptr_t)userData != gGeneration)
      return EM_TRUE;
   gOpen = false;
   gWs = 0;
   gOpenedAt = 0;
   logprintf(LogConsumer::LogUDP, "UDP relay websocket closed; retrying");
   scheduleRetry();
   return EM_TRUE;
}

EM_BOOL onError(int, const EmscriptenWebSocketErrorEvent *, void *userData)
{
   if((int)(intptr_t)userData != gGeneration)
      return EM_TRUE;
   logprintf(LogConsumer::LogError, "UDP relay websocket error (%s); retrying", gUrl.c_str());
   scheduleRetry();
   return EM_TRUE;
}

EM_BOOL onMessage(int, const EmscriptenWebSocketMessageEvent *event, void *userData)
{
   if((int)(intptr_t)userData != gGeneration)
      return EM_TRUE;
   if(event->isText)
      return EM_TRUE;
   handleFrame(event->data, event->numBytes);
   return EM_TRUE;
}

void connectNow()
{
   if(gUrl.empty())
      return;
   if(gFirstAttempt == 0)
      gFirstAttempt = emscripten_get_now();
   gOpenedAt = 0;
   if(gWs)
   {
      EMSCRIPTEN_WEBSOCKET_T old = gWs;
      gWs = 0;
      gOpen = false;
      emscripten_websocket_close(old, 1000, 0);
      emscripten_websocket_delete(old);
   }
   gGeneration++;
   EmscriptenWebSocketCreateAttributes attr;
   emscripten_websocket_init_create_attributes(&attr);
   attr.url = gUrl.c_str();
   gWs = emscripten_websocket_new(&attr);
   if(!gWs)
   {
      logprintf(LogConsumer::LogError, "UDP relay websocket create failed (%s)", gUrl.c_str());
      scheduleRetry();
      return;
   }
   void *generation = (void *)(intptr_t)gGeneration;
   emscripten_websocket_set_onopen_callback(gWs, generation, onOpen);
   emscripten_websocket_set_onclose_callback(gWs, generation, onClose);
   emscripten_websocket_set_onerror_callback(gWs, generation, onError);
   emscripten_websocket_set_onmessage_callback(gWs, generation, onMessage);
   logprintf(LogConsumer::LogUDP, "UDP relay connecting: %s", gUrl.c_str());
}

void ensureSocket()
{
   if(gWs || gOpen)
      return;
   connectNow();
}

std::string readRelayUrl()
{
   char buf[512];
   buf[0] = 0;
   EM_ASM({
      var q = '';
      var params = null;
      try { params = new URLSearchParams(window.location.search); } catch (e) {}
      // ?relay= is explicit. Empty, "off", or "0" disables. Anything else
      // replaces the default. With no query param, Module.relay wins, then
      // the hosted relay.
      if (params && params.has('relay')) {
         q = params.get('relay') || '';
         if (q === 'off' || q === 'none' || q === '0')
            q = '';
      } else if (typeof Module !== 'undefined' && Module['relay']) {
         q = Module['relay'];
      } else {
         q = UTF8ToString($2);
      }
      stringToUTF8(q, $0, $1);
   }, buf, (int)sizeof(buf), DEFAULT_RELAY);
   return std::string(buf);
}

}

U32 TNL::emscriptenRelayMasterWaitMs(U32 gameUnconnectedMs)
{
   if(!emscriptenRelayEnabled())
      return gameUnconnectedMs;
   // Still dialing. The server browser keeps saying "Contacting master".
   if(gGaveUp)
      return gameUnconnectedMs > 8000 ? gameUnconnectedMs : 8001;
   if(!gOpen || gOpenedAt == 0.0)
      return 0;

   double age = emscripten_get_now() - gOpenedAt;
   if(age < 0)
      age = 0;
   U32 sinceOpen = age > 1.0e9 ? 1000000000u : U32(age);
   // gameUnconnectedMs is 0 while the master connection is up, and it restarts
   // if that connection drops. Don't let a long-open socket replace either.
   return gameUnconnectedMs < sinceOpen ? gameUnconnectedMs : sinceOpen;
}

void TNL::emscriptenRelayAllow(U32 ip, U16 port)
{
   if(!ip || !port || !emscriptenRelayEnabled())
      return;
   ensureSocket();
   std::vector<U8> frame(7);
   frame[0] = ALLOW;
   writeU32(&frame[1], ip);
   writeU16(&frame[5], port);
   sendFrame(frame);
}

bool TNL::emscriptenRelayEnabled()
{
   if(!gUrlReady)
   {
      gUrl = readRelayUrl();
      gUrlReady = true;
      if(!gUrl.empty())
         logprintf(LogConsumer::LogUDP, "UDP relay configured: %s", gUrl.c_str());
   }
   return !gUrl.empty();
}

bool TNL::emscriptenRelayBind(S32 &outFd, U16 port)
{
   if(!emscriptenRelayEnabled())
      return false;
   ensureSocket();
   RelaySock sock;
   sock.id = gNextId++;
   sock.bound = false;
   gSocks[sock.id] = sock;

   std::vector<U8> frame(7);
   frame[0] = BIND;
   writeU32(&frame[1], sock.id);
   writeU16(&frame[5], port);
   sendFrame(frame);

   outFd = S32(sock.id);
   return true;
}

void TNL::emscriptenRelayClose(S32 fd)
{
   gSocks.erase(U32(fd));
}

bool TNL::emscriptenRelayOwns(S32 fd)
{
   return fd > 0 && gSocks.find(U32(fd)) != gSocks.end();
}

NetError TNL::emscriptenRelaySendTo(S32 fd, const Address &address, const U8 *buffer, S32 bufferSize)
{
   std::map<U32, RelaySock>::iterator it = gSocks.find(U32(fd));
   if(it == gSocks.end())
      return InvalidPacketProtocol;
   if(bufferSize < 0 || bufferSize > MAX_PAYLOAD)
      return InvalidPacketProtocol;

   std::vector<U8> frame(13 + bufferSize);
   frame[0] = SEND;
   writeU32(&frame[1], it->second.id);
   writeU32(&frame[5], address.netNum[0]);
   writeU16(&frame[9], address.port);
   writeU16(&frame[11], U16(bufferSize));
   if(bufferSize)
      memcpy(&frame[13], buffer, bufferSize);

   if(!it->second.bound)
      it->second.waiting.push_back(frame);
   else
      sendFrame(frame);
   return NoError;
}

NetError TNL::emscriptenRelayRecvFrom(S32 fd, Address *address, U8 *buffer, S32 bufferSize, S32 *outSize)
{
   std::map<U32, RelaySock>::iterator it = gSocks.find(U32(fd));
   if(it == gSocks.end() || it->second.inbox.empty())
      return WouldBlock;
   Packet packet = it->second.inbox.front();
   it->second.inbox.pop_front();
   if(S32(packet.data.size()) > bufferSize)
      return InvalidPacketProtocol;
   address->transport = IPProtocol;
   address->port = packet.port;
   address->netNum[0] = packet.ip;
   address->netNum[1] = 0;
   address->netNum[2] = 0;
   address->netNum[3] = 0;
   if(!packet.data.empty())
      memcpy(buffer, &packet.data[0], packet.data.size());
   *outSize = S32(packet.data.size());
   return NoError;
}
