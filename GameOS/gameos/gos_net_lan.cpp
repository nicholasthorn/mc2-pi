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
#include <ifaddrs.h>
#include <net/if.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
typedef socklen_t socklen_type;
typedef const void* send_data_type;
typedef void* receive_data_type;
#endif

#include <cctype>
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

// an address another machine could use: not none, the loopback or link-local (169.254)
bool isUsable(uint32_t ip)
{
	return ip != 0 && (ip >> 24) != 127u && (ip >> 16) != 0xA9FEu;
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
		const int error = WSAGetLastError();
		// a send to a port nobody is on comes back as this, and says nothing about what is waiting
		if (error == WSAECONNRESET)
			return receive(out, capacity, size, from);
		// a datagram larger than the buffer is an error there, not a short read
		if (error != WSAEMSGSIZE)
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

bool LanSocket::parseAddress(const char* text, uint32_t& ip)
{
	// By hand, not inet_pton: the TCP/IP screen's box lets a player type spaces round the
	// address and zeros in front of a number ("192.168.086.076"), which inet_pton refuses.
	if (!text)
		return false;
	while (*text == ' ')
		text++;
	uint32_t result = 0;
	for (int part = 0; part < 4; part++) {
		if (part > 0 && *text++ != '.')
			return false;
		uint32_t number = 0;
		int digits = 0;
		for (; std::isdigit(static_cast<unsigned char>(*text)); text++) {
			number = number * 10u + static_cast<uint32_t>(*text - '0');
			if (++digits > 3)
				return false;
		}
		if (digits == 0 || number > 255u)
			return false;
		result = (result << 8) | number;
	}
	while (*text == ' ')
		text++;
	if (*text)
		return false;
	ip = result;
	return true;
}

uint32_t LanSocket::localAddress()
{
	if (!startSockets())
		return 0;
	// A connected UDP socket sends nothing, but has the address the system would send from.
	// Any address beyond the local network will do: this is a public DNS server's.
#ifdef PLATFORM_WINDOWS
	const SOCKET made = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (made == INVALID_SOCKET)
		return 0;
#else
	const int made = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (made < 0)
		return 0;
#endif
	sockaddr_in address = toSockaddr(0x08080808u, 53);
	socklen_type length = sizeof(address);
	uint32_t ip = 0;
	if (connect(made, reinterpret_cast<const sockaddr*>(&address), sizeof(address)) == 0
		&& getsockname(made, reinterpret_cast<sockaddr*>(&address), &length) == 0)
		ip = ntohl(address.sin_addr.s_addr);
	closeSocket(static_cast<intptr_t>(made));
	if (isUsable(ip))
		return ip;
	// No default route (a router with no internet behind it, two machines on a cable): the
	// first address of an interface that is up, not the loopback or a link-local one.
	ip = 0;
#ifdef PLATFORM_WINDOWS
	char name[256];
	addrinfo hints;
	std::memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	addrinfo* found = nullptr;
	if (gethostname(name, sizeof(name)) == 0 && getaddrinfo(name, nullptr, &hints, &found) == 0) {
		for (const addrinfo* a = found; a && !ip; a = a->ai_next) {
			const uint32_t candidate = ntohl(reinterpret_cast<const sockaddr_in*>(a->ai_addr)->sin_addr.s_addr);
			if (isUsable(candidate))
				ip = candidate;
		}
		freeaddrinfo(found);
	}
#else
	ifaddrs* interfaces = nullptr;
	if (getifaddrs(&interfaces) == 0) {
		for (const ifaddrs* i = interfaces; i && !ip; i = i->ifa_next) {
			if (!i->ifa_addr || i->ifa_addr->sa_family != AF_INET || !(i->ifa_flags & IFF_UP))
				continue;
			const uint32_t candidate = ntohl(reinterpret_cast<const sockaddr_in*>(i->ifa_addr)->sin_addr.s_addr);
			if (isUsable(candidate))
				ip = candidate;
		}
		freeifaddrs(interfaces);
	}
#endif
	return ip;
}

bool runningInWsl()
{
#ifdef PLATFORM_WINDOWS
	return false;
#else
	FILE* file = std::fopen("/proc/sys/kernel/osrelease", "r");
	if (!file)
		return false;
	char text[256] = {};
	const bool read = std::fgets(text, sizeof(text), file) != nullptr;
	std::fclose(file);
	if (!read)
		return false;
	for (char* c = text; *c; c++)
		*c = static_cast<char>(std::tolower(static_cast<unsigned char>(*c)));
	return std::strstr(text, "microsoft") != nullptr;
#endif
}

bool wslBehindNat()
{
#ifdef PLATFORM_WINDOWS
	return false;
#else
	if (!runningInWsl())
		return false;
	// WSL's own tool says which; one too old to know is NAT, the only kind it had
	FILE* tool = popen("wslinfo --networking-mode 2>/dev/null", "r");
	if (!tool)
		return true;
	char mode[64] = {};
	const bool read = std::fgets(mode, sizeof(mode), tool) != nullptr;
	pclose(tool);
	return !read || std::strncmp(mode, "mirrored", 8) != 0;
#endif
}

}
