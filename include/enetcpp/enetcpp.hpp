/*

This is free and unencumbered software released into the public domain.

Anyone is free to copy, modify, publish, use, compile, sell, or
distribute this software, either in source code form or as a compiled
binary, for any purpose, commercial or non-commercial, and by any
means.

In jurisdictions that recognize copyright laws, the author or authors
of this software dedicate any and all copyright interest in the
software to the public domain. We make this dedication for the benefit
of the public at large and to the detriment of our heirs and
successors. We intend this dedication to be an overt act of
relinquishment in perpetuity of all present and future rights to this
software under copyright law.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR
OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE.

For more information, please refer to <https://unlicense.org>

 */

/**
 * @file enetcpp.hpp
 * @brief A C++ wrapper for the ENet networking library.
 *
 * This file contains the declaration of classes that wrap the ENet C library,
 * providing a more modern C++ interface for creating and managing network
 * connections, sending and receiving packets, and handling events such as
 * connections, disconnections, and data transmission.
 *
 * ENet is designed to handle reliable and unreliable UDP communication, and
 * this wrapper makes it easier to work with ENet in C++ by leveraging RAII
 * principles and modern C++ features.
 *
 * Classes provided in this wrapper include:
 *  - @ref enetcpp::Address: Represents a network address.
 *  - @ref enetcpp::Packet: Manages ENet packets for data transmission.
 *  - @ref enetcpp::Peer: Represents a network peer.
 *  - @ref enetcpp::Event: Handles network events, such as connection,
 * disconnection, and data reception.
 *  - @ref enetcpp::Host: Manages the creation of client and server hosts.
 *
 * @note This wrapper requires the ENet library to be installed and properly
 * linked. See `readme.md` for instructions, or just copy this file into your
 * project.
 *
 * @see https://github.com/lsalzman/enet for more information about ENet.
 */

#ifndef _ENETCPP_ENETCPP_HPP_
#define _ENETCPP_ENETCPP_HPP_

#include <cstdlib>
#include <enet/enet.h>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace enetcpp {

/** @brief Type alias for ENet 8-bit unsigned integer */
typedef enet_uint8 uint8;

/** @brief Type alias for ENet 16-bit unsigned integer */
typedef enet_uint16 uint16;

/** @brief Type alias for ENet 32-bit unsigned integer */
typedef enet_uint32 uint32;

/**
 * @brief Wrapper class for ENetAddress.
 *
 * Provides convenience methods for handling network addresses in ENet.
 */
class Address {
  private:
    ENetAddress m_address;

  public:
    /**
     * @brief Default constructor for Address.
     */
    Address() {}

    /**
     * @brief Constructs an Address from a host and port.
     * @param host The hostname or IP address as a string.
     * @param port The port number.
     */
    Address(std::string host, uint16 port) {
        enet_address_set_host(&m_address, host.c_str());
        m_address.port = port;
    }

    /**
     * @brief Constructs an Address from an integer host and port.
     * @param host The host address in integer form.
     * @param port The port number.
     */
    Address(uint32 host, uint16 port) {
        m_address.host = host;
        m_address.port = port;
    }

    /**
     * @brief Constructs an Address that binds to any available host.
     * @param port The port number.
     */
    Address(uint16 port) {
        m_address.host = ENET_HOST_ANY;
        m_address.port = port;
    }

    /**
     * @brief Retrieves the port of the address.
     * @return The port number.
     */
    uint16 port() const { return m_address.port; }

    /**
     * @brief Retrieves the host address in integer form.
     * @return The host address.
     */
    uint32 host() const { return m_address.host; }

    /**
     * @brief Retrieves the host address as a string.
     * @return The host address in string form.
     */
    std::string host_string() const {
        char out[32];
        enet_address_get_host(&m_address, out, 31);
        return std::string(out);
    }

    /**
     * @brief Returns a pointer to the underlying ENetAddress.
     * @return A pointer to the ENetAddress.
     */
    ENetAddress* get() { return &m_address; }
};

/**
 * @brief Wrapper class for ENetPacket.
 *
 * Manages the lifecycle of an ENet packet and provides access to its data and
 * flags.
 */
class Packet {
  private:
    ENetPacket* m_packet;
    bool m_dont_free = false;

  public:
    /**
     * @brief Constructs a Packet from an existing ENetPacket.
     * @param packet Pointer to an existing ENetPacket.
     */
    Packet(ENetPacket* packet) : m_packet(packet) {}

    /**
     * @brief Creates a new Packet with data.
     * @param data Pointer to the data to be included in the packet.
     * @param length The length of the data.
     * @param flags Flags for packet reliability and other options.
     */
    Packet(const void* data, size_t length,
           uint32 flags = ENET_PACKET_FLAG_RELIABLE)
        : m_packet(enet_packet_create(data, length, flags)) {}

    /**
     * @brief Prevents the packet from being freed when destroyed.
     */
    void set_dont_free() { m_dont_free = true; }

    /**
     * @brief Destroys the packet unless flagged with `set_dont_free()`.
     */
    ~Packet() {
        if (!m_dont_free)
            enet_packet_destroy(m_packet);
    }

    /**
     * @brief Accesses the packet's data.
     * @return A pointer to the packet data.
     */
    void* data() { return m_packet->data; }

    /**
     * @brief Accesses the packet's data (const version).
     * @return A const pointer to the packet data.
     */
    const void* data() const { return m_packet->data; }

    /**
     * @brief Retrieves the length of the packet's data.
     * @return The length of the packet data.
     */
    size_t length() const { return m_packet->dataLength; }

    /**
     * @brief Retrieves the packet's flags.
     * @return The packet's flags.
     */
    uint32 flags() const { return m_packet->flags; }

    /**
     * @brief Returns the underlying ENetPacket pointer.
     * @return A pointer to the ENetPacket.
     */
    ENetPacket* get() { return m_packet; }
};

/**
 * @brief Wrapper class for ENetPeer.
 *
 * Provides functionality to manage peers in ENet, such as sending data and
 * disconnecting.
 */
class Peer {
  private:
    ENetPeer* m_peer;

  public:
    /**
     * @brief Constructs a Peer from an ENetPeer pointer.
     * @param peer Pointer to an existing ENetPeer.
     */
    Peer(ENetPeer* peer) : m_peer(peer) {}

    /**
     * @brief Disconnects the peer.
     */
    void disconnect() { enet_peer_disconnect(m_peer, 0); }

    /**
     * @brief Resets the peer to its initial state.
     */
    void reset() { enet_peer_reset(m_peer); }

    /**
     * @brief Sends a packet to the peer.
     * @param packet Reference to the Packet to send.
     * @throws std::runtime_error if the packet send fails.
     */
    void send(Packet& packet) {
        if (enet_peer_send(m_peer, 0, packet.get()) == 0) {
            packet.set_dont_free();
            return;
        }
        throw std::runtime_error("Packet send failed");
    }
};

/**
 * @brief Wrapper class for ENetEvent.
 *
 * Manages event data, including peer and address information.
 */
class Event {
  private:
    ENetEvent m_event;
    Address m_address;
    Peer m_peer;

  public:
    /**
     * @brief Constructs an Event from an ENetEvent.
     * @param event The ENetEvent.
     */
    Event(ENetEvent event)
        : m_event(event),
          m_address(m_event.peer->address.host, m_event.peer->address.port),
          m_peer(m_event.peer) {}

    /**
     * @brief Retrieves the address associated with the event.
     * @return A reference to the Address object.
     */
    const Address& address() const { return m_address; }

    /**
     * @brief Accesses peer-specific data.
     * @return A pointer to the peer data.
     */
    void* peer_data() { return m_event.peer->data; }

    /**
     * @brief Accesses peer-specific data (const version).
     * @return A const pointer to the peer data.
     */
    const void* peer_data() const { return m_event.peer->data; }

    /**
     * @brief Sets peer-specific data.
     * @param data Pointer to the data to associate with the peer.
     */
    void set_peer_data(void* data) { m_event.peer->data = data; }

    /**
     * @brief Retrieves the channel ID associated with the event.
     * @return The channel ID.
     */
    uint8 channel() const { return m_event.channelID; }

    /**
     * @brief Retrieves the peer associated with the event.
     * @return A reference to the Peer object.
     */
    Peer& peer() { return m_peer; }
};

/**
 * @brief Specialized event class for connection events.
 */
class EventConnect : public Event {
  public:
    using Event::Event;
};

/**
 * @brief Specialized event class for disconnection events.
 */
class EventDisconnect : public Event {
  public:
    using Event::Event;
};

/**
 * @brief Specialized event class for data reception events.
 *
 * Handles receiving packets from peers.
 */
class EventReceive : public Event {
  private:
    Packet m_packet;

  public:
    /**
     * @brief Constructs an EventReceive from an ENetEvent.
     * @param event The ENetEvent.
     */
    EventReceive(ENetEvent event) : Event(event), m_packet(event.packet) {}

    /**
     * @brief Retrieves the packet received in the event.
     * @return A reference to the Packet object.
     */
    Packet& packet() { return m_packet; }

    /**
     * @brief Retrieves the packet received in the event (const version).
     * @return A const reference to the Packet object.
     */
    const Packet& packet() const { return m_packet; }
};

/**
 * @brief Wrapper class for ENetHost.
 *
 * Manages the creation of ENet hosts, both for servers and clients.
 */
class Host {
  private:
    Address m_address;
    ENetHost* m_host;
    bool m_is_server;
    std::vector<ENetPeer*> m_peers;

    /**
     * @brief Dispatches an event to the appropriate handler.
     * @tparam EventType The type of event to handle.
     * @param event_ The ENetEvent to dispatch.
     */
    template <class EventType> void dispatch(ENetEvent event_) {
        EventType event(event_);
        this->on_event(event);
    }

  public:
    /**
     * @brief Constructs a server Host with the specified address and
     * configuration.
     * @param address The server address.
     * @param peer_count The maximum number of peers.
     * @param channel_limit The maximum number of channels.
     * @param incoming_bandwidth The incoming bandwidth limit.
     * @param outgoing_bandwidth The outgoing bandwidth limit.
     * @throws std::runtime_error if the host creation fails.
     */
    Host(Address address, size_t peer_count, size_t channel_limit = 1U,
         uint32 incoming_bandwith = 0U, uint32 outgoing_bandwidth = 0U)
        : m_address(address), m_is_server(true) {
        m_host = enet_host_create(m_address.get(), peer_count, channel_limit,
                                  incoming_bandwith, outgoing_bandwidth);
        if (m_host == NULL) {
            throw std::runtime_error("Failed to create an ENet server host");
        }
    }

    /**
     * @brief Constructs a client Host with the specified configuration.
     * @param peer_count The maximum number of peers.
     * @param channel_limit The maximum number of channels.
     * @param incoming_bandwidth The incoming bandwidth limit.
     * @param outgoing_bandwidth The outgoing bandwidth limit.
     * @throws std::runtime_error if the host creation fails.
     */
    Host(size_t peer_count, size_t channel_limit = 1U,
         uint32 incoming_bandwith = 0U, uint32 outgoing_bandwidth = 0U)
        : m_is_server(false) {
        m_host = enet_host_create(NULL, peer_count, channel_limit,
                                  incoming_bandwith, outgoing_bandwidth);
        if (m_host == NULL) {
            throw std::runtime_error("Failed to create an ENet client host");
        }
    }

    /**
     * @brief Destructor for Host.
     *
     * Destroys the underlying ENetHost object.
     */
    ~Host() { enet_host_destroy(m_host); }

    /**
     * @brief Services the host to check for network events.
     * @param timeout The maximum time to wait for an event, in milliseconds.
     * @return The result of the ENet service call.
     */
    int service(uint32 timeout = 0) {
        ENetEvent event;
        int rc = enet_host_service(m_host, &event, timeout);
        if (rc > 0) {
            switch (event.type) {
            case ENET_EVENT_TYPE_CONNECT:
                dispatch<EventConnect>(event);
                break;
            case ENET_EVENT_TYPE_DISCONNECT:
                dispatch<EventDisconnect>(event);
                break;
            case ENET_EVENT_TYPE_RECEIVE:
                dispatch<EventReceive>(event);
                break;
            default:
                break;
            }
        }
        return rc;
    }

    /**
     * @brief Initiates a connection to a remote address.
     * @param address The remote Address to connect to.
     * @param channels The number of channels to use.
     * @param data Optional data to associate with the connection.
     * @param timeout The time to wait for a connection.
     * @return A Peer representing the connection.
     * @throws std::runtime_error if the connection fails.
     */
    Peer connect(Address address, size_t channels = 1, uint32 data = 0,
                 uint32 timeout = 5000) {
        ENetPeer* peer =
            enet_host_connect(m_host, address.get(), channels, data);
        if (peer == NULL) {
            throw std::runtime_error(
                "No available peers for initiating an ENet connection.");
        }
        ENetEvent event;
        if (!((enet_host_service(m_host, &event, timeout) > 0) &&
              (event.type == ENET_EVENT_TYPE_CONNECT))) {
            enet_peer_reset(peer);
            throw std::runtime_error("Connection failed");
        }
        flush();
        m_peers.push_back(peer);
        return Peer(peer);
    }

    /**
     * @brief Flushes any queued packets to the network.
     */
    void flush() { enet_host_flush(m_host); }

    /**
     * @brief Handles connection events.
     * @param event The connection event.
     */
    virtual void on_event(EventConnect& event) {
        std::cout << "A new client connected from "
                  << event.address().host_string() << ":"
                  << event.address().port() << std::endl;
    }

    /**
     * @brief Handles disconnection events.
     * @param event The disconnection event.
     */
    virtual void on_event(EventDisconnect& event) {
        std::cout << "Client " << event.address().host_string() << ":"
                  << event.address().port() << " disconnected" << std::endl;
    }

    /**
     * @brief Handles packet reception events.
     * @param event The packet reception event.
     */
    virtual void on_event(EventReceive& event) {
        std::cout << "A packet of length " << event.packet().length()
                  << " was received from " << event.address().host_string()
                  << ":" << event.address().port() << "on channel "
                  << event.channel() << std::endl;
    }
};

} // namespace enetcpp

#endif // _ENETCPP_ENETCPP_HPP_