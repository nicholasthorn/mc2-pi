// GameOS/gameos/gos_net_lan.h
//
// A plain UDP socket, for finding games on the local network: the transport in gos_net.h
// connects to an address it already has, and has no broadcast. Never blocks. What it
// carries is code/netwire.h's LanQuery and LanAnnounce, and only those.

#ifndef GOS_NET_LAN_H
#define GOS_NET_LAN_H

#include <cstddef>
#include <cstdint>
#include <string>

namespace gos_net {

struct LanAddress {
	uint32_t ip = 0;		// host byte order
	uint16_t port = 0;
};

class LanSocket {
public:
	LanSocket() {}
	~LanSocket() { close(); }
	LanSocket(const LanSocket&) = delete;
	LanSocket& operator=(const LanSocket&) = delete;

	// Port 0 takes any free port. False if the port is taken, or there are no sockets.
	bool open(uint16_t port);
	void close();
	bool isOpen() const { return open_; }
	uint16_t port() const { return port_; }

	bool sendTo(const LanAddress& to, const void* data, size_t size);
	// To the whole local network, and to this machine, as a host on the same machine must be
	// found whatever the router or the network mode lets through. True if either went out.
	bool broadcast(uint16_t port, const void* data, size_t size);

	// False if nothing is waiting. A datagram larger than capacity is cut short and comes
	// back as received: the decoder refuses it.
	bool receive(void* out, size_t capacity, size_t& size, LanAddress& from);

	static std::string addressText(uint32_t ip);
	// Dotted text such as "192.168.1.20" to an address. False if it is not one.
	static bool parseAddress(const char* text, uint32_t& ip);
	// This machine's address on the network it would reach others through (the interface
	// with the default route), for a host to show those who would join it. Sends nothing.
	// 0 with no network.
	static uint32_t localAddress();

private:
	intptr_t socket_ = -1;
	bool open_ = false;
	uint16_t port_ = 0;
};

// True in WSL (Windows Subsystem for Linux). Its usual (NAT) networking hides the game from
// other machines: they cannot reach it, its broadcasts do not reach them, and its address
// works only from Windows on the same PC. Mirrored networking (.wslconfig) shares Windows'.
bool runningInWsl();

}

#endif
