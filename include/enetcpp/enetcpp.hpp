#ifndef _ENETCPP_ENETCPP_HPP_
#define _ENETCPP_ENETCPP_HPP_

#include <cstdlib>
#include <enet/enet.h>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace enetcpp {

typedef enet_uint16 uint16;
typedef enet_uint32 uint32;
typedef enet_uint8 uint8;

class Address {
  private:
    ENetAddress m_address;

  public:
    Address() {}

    Address(std::string host, uint16 port) {
        enet_address_set_host(&m_address, host.c_str());
        m_address.port = port;
    }

    Address(uint32 host, uint16 port) {
        m_address.host = host;
        m_address.port = port;
    }

    Address(uint16 port) {
        m_address.host = ENET_HOST_ANY;
        m_address.port = port;
    }

    uint16 port() const { return m_address.port; }

    uint32 host() const { return m_address.host; }

    std::string host_ip() const {
        char out[32];
        enet_address_get_host(&m_address, out, 31);
        return std::string(out);
    }

    ENetAddress* get() { return &m_address; }
};

class Packet {
  private:
    ENetPacket* m_packet;
    bool m_dont_free = false;

  public:
    Packet(ENetPacket* packet) : m_packet(packet) {}

    Packet(const void* data, size_t length,
           uint32 flags = ENET_PACKET_FLAG_RELIABLE)
        : m_packet(enet_packet_create(data, length, flags)) {}

    void set_dont_free() { m_dont_free = true; }

    ~Packet() {
        if (!m_dont_free)
            enet_packet_destroy(m_packet);
    }

    void* data() { return m_packet->data; }

    const void* data() const { return m_packet->data; }

    size_t length() const { return m_packet->dataLength; }

    uint32 flags() const { return m_packet->flags; }

    ENetPacket* get() { return m_packet; }
};

class Peer {
  private:
    ENetPeer* m_peer;

  public:
    Peer(ENetPeer* peer) : m_peer(peer) {}

    void disconnect() { enet_peer_disconnect(m_peer, 0); }

    void reset() { enet_peer_reset(m_peer); }

    void send(Packet& packet) {
        if (enet_peer_send(m_peer, 0, packet.get()) == 0) {
            packet.set_dont_free();
            return;
        }
        throw std::runtime_error("Packet send failed");
    }
};

class Event {
  private:
    ENetEvent m_event;
    Address m_address;
    Peer m_peer;

  public:
    Event(ENetEvent event)
        : m_event(event),
          m_address(m_event.peer->address.host, m_event.peer->address.port),
          m_peer(m_event.peer) {}

    const Address& address() const { return m_address; }

    void* peer_data() { return m_event.peer->data; }

    const void* peer_data() const { return m_event.peer->data; }

    void set_peer_data(void* data) { m_event.peer->data = data; }

    uint8 channel() const { return m_event.channelID; }

    Peer& peer() { return m_peer; }
};

class EventConnect : public Event {
  public:
    using Event::Event;
};

class EventDisconnect : public Event {
  public:
    using Event::Event;
};

class EventReceive : public Event {
  private:
    Packet m_packet;

  public:
    EventReceive(ENetEvent event) : Event(event), m_packet(event.packet) {}

    Packet& packet() { return m_packet; }

    const Packet& packet() const { return m_packet; }
};

class Host {
  private:
    Address m_address;
    ENetHost* m_host;
    bool m_is_server;
    std::vector<ENetPeer*> m_peers;

    template <class EventType> void dispatch(ENetEvent event_) {
        EventType event(event_);
        this->on_event(event);
    }

  public:
    Host(Address address, size_t peer_count, size_t channel_limit = 1U,
         uint32 incoming_bandwith = 0U, uint32 outgoing_bandwidth = 0U)
        : m_address(address), m_is_server(true) {
        m_host = enet_host_create(m_address.get(), peer_count, channel_limit,
                                  incoming_bandwith, outgoing_bandwidth);
        if (m_host == NULL) {
            throw std::runtime_error("Failed to create an ENet server host");
        }
    }

    Host(size_t peer_count, size_t channel_limit = 1U,
         uint32 incoming_bandwith = 0U, uint32 outgoing_bandwidth = 0U)
        : m_is_server(false) {
        m_host = enet_host_create(NULL, peer_count, channel_limit,
                                  incoming_bandwith, outgoing_bandwidth);
        if (m_host == NULL) {
            throw std::runtime_error("Failed to create an ENet client host");
        }
    }

    ~Host() { enet_host_destroy(m_host); }

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

    void flush() { enet_host_flush(m_host); }

    virtual void on_event(EventConnect& event) {
        std::cout << "A new client connected from " << event.address().host_ip()
                  << ":" << event.address().port() << std::endl;
    }

    virtual void on_event(EventDisconnect& event) {
        std::cout << "Client " << event.address().host_ip() << ":"
                  << event.address().port() << " disconnected" << std::endl;
    }

    virtual void on_event(EventReceive& event) {
        std::cout << "A packet of length " << event.packet().length()
                  << " was received from " << event.address().host_ip() << ":"
                  << event.address().port() << "on channel " << event.channel()
                  << std::endl;
    }
};

} // namespace enetcpp

#endif // _ENETCPP_ENETCPP_HPP_