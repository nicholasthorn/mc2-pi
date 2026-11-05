// GameOS/gameos/gos_net_lan.cpp: see gos_net_lan.h

#include "gos_net_lan.h"

#ifdef PLATFORM_WINDOWS
#include <winsock2.h>
#include <ws2tcpip.h>
typedef int socklen_type;
typedef const char* send_data_type;
typedef char* receive_data_type;
#else
#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
typedef socklen_t socklen_type;
typedef const void* send_data_type;
typedef void* receive_data_type;
#endif

#include <cstdio>
#include <cstring>

namespace gos_net {

namespace {

bool startSockets()
{
#ifdef PLATFORM_WINDOWS
	static bool started = false;
	if (!started) {
		WSADATA data;
		started = (WSAStartup(MAKEWORD(2, 2), &data) == 0);
	}
	return started;
#else
	return true;
#endif
}

void closeSocket(intptr_t s)
{
#ifdef PLATFORM_WINDOWS
	closesocket(static_cast<SOCKET>(s));
#else
	::close(static_cast<int>(s));
#endif
}

bool setNonBlocking(intptr_t s)
{
#ifdef PLATFORM_WINDOWS
	u_long on = 1;
	return ioctlsocket(static_cast<SOCKET>(s), FIONBIO, &on) == 0;
#else
	const int flags = fcntl(static_cast<int>(s), F_GETFL, 0);
	return flags >= 0 && fcntl(static_cast<int>(s), F_SETFL, flags | O_NONBLOCK) == 0;
#endif
}

sockaddr_in toSockaddr(uint32_t ip, uint16_t port)
{
	sockaddr_in address;
	std::memset(&address, 0, sizeof(address));
	address.sin_family = AF_INET;
	address.sin_addr.s_addr = htonl(ip);
	address.sin_port = htons(port);
	return address;
}

}

bool LanSocket::open(uint16_t port)
{
	close();
	if (!startSockets())
		return false;
#ifdef PLATFORM_WINDOWS
	const SOCKET made = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (made == INVALID_SOCKET)
		return false;
#else
	const int made = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (made < 0)
		return false;
#endif
	const intptr_t s = static_cast<intptr_t>(made);
	const int on = 1;
	sockaddr_in address = toSockaddr(INADDR_ANY, port);
	socklen_type length = sizeof(address);
	if (setsockopt(made, SOL_SOCKET, SO_BROADCAST, reinterpret_cast<const char*>(&on), sizeof(on)) != 0
		|| !setNonBlocking(s)
		|| bind(made, reinterpret_cast<const sockaddr*>(&address), sizeof(address)) != 0
		|| getsockname(made, reinterpret_cast<sockaddr*>(&address), &length) != 0) {
		closeSocket(s);
		return false;
	}
	socket_ = s;
	port_ = ntohs(address.sin_port);
	open_ = true;
	return true;
}

void LanSocket::close()
{
	if (open_)
		closeSocket(socket_);
	open_ = false;
	socket_ = -1;
	port_ = 0;
}

bool LanSocket::sendTo(const LanAddress& to, const void* data, size_t size)
{
	if (!open_)
		return false;
	const sockaddr_in address = toSockaddr(to.ip, to.port);
#ifdef PLATFORM_WINDOWS
	const SOCKET s = static_cast<SOCKET>(socket_);
	const int count = static_cast<int>(size);
#else
	const int s = static_cast<int>(socket_);
	const size_t count = size;
#endif
	return sendto(s, static_cast<send_data_type>(data), count, 0, reinterpret_cast<const sockaddr*>(&address), sizeof(address)) == static_cast<long>(size);
}

bool LanSocket::broadcast(uint16_t port, const void* data, size_t size)
{
	LanAddress to;
	to.port = port;
	to.ip = INADDR_BROADCAST;
	const bool everyone = sendTo(to, data, size);
	to.ip = INADDR_LOOPBACK;
	const bool here = sendTo(to, data, size);
	return everyone || here;
}

bool LanSocket::receive(void* out, size_t capacity, size_t& size, LanAddress& from)
{
	if (!open_)
		return false;
	sockaddr_in address;
	socklen_type length = sizeof(address);
#ifdef PLATFORM_WINDOWS
	const SOCKET s = static_cast<SOCKET>(socket_);
	const int room = static_cast<int>(capacity);
#else
	const int s = static_cast<int>(socket_);
	const size_t room = capacity;
#endif
	const long got = static_cast<long>(recvfrom(s, static_cast<receive_data_type>(out), room, 0, reinterpret_cast<sockaddr*>(&address), &length));
	if (got < 0) {
#ifdef PLATFORM_WINDOWS
		// a datagram larger than the buffer is an error there, not a short read
		if (WSAGetLastError() != WSAEMSGSIZE)
			return false;
		size = capacity;
#else
		return false;
#endif
	}
	else
		size = static_cast<size_t>(got);
	from.ip = ntohl(address.sin_addr.s_addr);
	from.port = ntohs(address.sin_port);
	return true;
}

std::string LanSocket::addressText(uint32_t ip)
{
	char text[32];
	std::snprintf(text, sizeof(text), "%u.%u.%u.%u", (ip >> 24) & 0xFFu, (ip >> 16) & 0xFFu, (ip >> 8) & 0xFFu, ip & 0xFFu);
	return text;
}

}
