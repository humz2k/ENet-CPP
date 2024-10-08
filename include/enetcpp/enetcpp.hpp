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
 *    disconnection, and data reception.
 *  - @ref enetcpp::Host: Manages the creation of client and server hosts.
 *  - @ref enetcpp::Logger: A simple logger class to provide tracing and
 *    debugging functionality.
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
#include <ctime>
#include <enet/enet.h>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace enetcpp {

/** @brief Type alias for ENet 8-bit unsigned integer */
using uint8 = enet_uint8;

/** @brief Type alias for ENet 16-bit unsigned integer */
using uint16 = enet_uint16;

/** @brief Type alias for ENet 32-bit unsigned integer */
using uint32 = enet_uint32;

/**
 * @brief A simple logger class for logging and tracing within the ENetCPP
 * wrapper.
 *
 * The Logger class allows logging of various events at different levels (TRACE,
 * DEBUG, INFO, MINIMAL). It provides methods for logging messages with
 * timestamps to help trace ENet operations.
 *
 * Logging levels:
 *  - TRACE: Logs detailed debug information.
 *  - DEBUG: Logs general debug information.
 *  - INFO: Logs important runtime information.
 *  - MINIMAL: Logs minimal runtime information (e.g., critical events).
 *  - NONE: No logging.
 */
class Logger {
  public:
    enum LogLevel { NONE, MINIMAL, INFO, DEBUG, TRACE };

    /**
     * @brief Constructs a Logger with the given log level.
     * @param loglevel The log level to use (defaults to INFO).
     */
    Logger(LogLevel loglevel = INFO) : m_loglevel(loglevel) {}

    /**
     * @brief Sets the log level for the logger.
     * @param level The desired log level.
     */
    void set_loglevel(LogLevel level) { m_loglevel = level; }

    /**
     * @brief Logs a TRACE level message.
     * @param fmt The format string (similar to printf).
     * @param args Arguments for the format string.
     */
    template <typename... Args> void trace(const char* fmt, Args... args) {
        log(TRACE, "TRACE", fmt, args...);
    }

    /**
     * @brief Logs a DEBUG level message.
     * @param fmt The format string (similar to printf).
     * @param args Arguments for the format string.
     */
    template <typename... Args> void debug(const char* fmt, Args... args) {
        log(DEBUG, "DEBUG", fmt, args...);
    }

    /**
     * @brief Logs an INFO level message.
     * @param fmt The format string (similar to printf).
     * @param args Arguments for the format string.
     */
    template <typename... Args> void info(const char* fmt, Args... args) {
        log(INFO, "INFO", fmt, args...);
    }

    /**
     * @brief Logs a MINIMAL level message.
     * @param fmt The format string (similar to printf).
     * @param args Arguments for the format string.
     */
    template <typename... Args> void minimal(const char* fmt, Args... args) {
        log(MINIMAL, "MINIMAL", fmt, args...);
    }

  private:
    LogLevel m_loglevel;

    /**
     * @brief Prints the current timestamp to the log.
     */
    void print_time() {
        time_t rawtime;
        struct tm* timeinfo;
        char buffer[80];
        time(&rawtime);
        timeinfo = localtime(&rawtime);
        strftime(buffer, sizeof(buffer), "%d-%m-%Y %H:%M:%S", timeinfo);
        printf("[%s] : ", buffer);
    }

    template <typename... Args>
    void log(LogLevel level, const char* tag, const char* fmt, Args... args) {
        if (m_loglevel < level)
            return;
        print_time();
        printf("%s: ", tag);
        printf(fmt, args...);
        printf("\n");
    }
};

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
    bool m_owns_memory = true;

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
     *
     * We need this because, in the event that the packet is sent successfully,
     * ENet will free the packet for us, and we want to prevent deep copying the
     * packet.
     *
     */
    void release_ownership() { m_owns_memory = false; }

    /**
     * @brief Destroys the packet unless flagged with `set_dont_free()`.
     */
    ~Packet() {
        if (m_owns_memory)
            enet_packet_destroy(m_packet);
    }

    void destroy() {
        release_ownership();
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
            packet.release_ownership();
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
    Logger m_logger;

    /**
     * @brief Dispatches an event to the appropriate handler.
     * @tparam EventType The type of event to handle.
     * @param event_ The ENetEvent to dispatch.
     */
    template <class EventType> void dispatch(ENetEvent event_) {
        EventType event(event_);
        this->on_event(event);
    }

  protected:
    std::mutex m_mutex;

  public:
    Logger& logger() { return m_logger; }

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
         uint32 incoming_bandwith = 0U, uint32 outgoing_bandwidth = 0U,
         Logger logger = Logger())
        : m_address(address), m_is_server(true), m_logger(logger) {
        m_logger.trace("creating ENet server host");
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
         uint32 incoming_bandwith = 0U, uint32 outgoing_bandwidth = 0U,
         Logger logger = Logger())
        : m_is_server(false), m_logger(logger) {
        m_logger.trace("creating ENet client host");
        m_host = enet_host_create(NULL, peer_count, channel_limit,
                                  incoming_bandwith, outgoing_bandwidth);
        if (m_host == NULL) {
            throw std::runtime_error("Failed to create an ENet client host");
        }
    }

    /**
     * @brief Destructor for Host.
     *
     * FLushes the host and then destroys the underlying ENetHost object.
     */
    ~Host() {
        m_logger.trace("destroying ENet host");
        flush();
        enet_host_destroy(m_host);
    }

    /**
     * @brief Services the host to check for network events.
     * @param timeout The maximum time to wait for an event, in milliseconds -
     * locks the mutex for `timeout` seconds.
     * @return The result of the ENet service call.
     */
    int service(uint32 timeout = 0) {
        m_logger.trace("servicing ENet host");
        ENetEvent event;
        int rc;
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            rc = enet_host_service(m_host, &event, timeout);
        }
        if (rc > 0) {
            switch (event.type) {
            case ENET_EVENT_TYPE_CONNECT:
                m_logger.info("%x:%u connected", event.peer->address.host,
                              event.peer->address.port);
                dispatch<EventConnect>(event);
                break;
            case ENET_EVENT_TYPE_DISCONNECT:
                m_logger.info("%x:%u disconnected", event.peer->address.host,
                              event.peer->address.port);
                dispatch<EventDisconnect>(event);
                break;
            case ENET_EVENT_TYPE_RECEIVE:
                m_logger.info(
                    "received %lu bytes from %x:%u", event.packet->dataLength,
                    event.peer->address.host, event.peer->address.port);
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
     *
     * This is thread safe.
     *
     * @param address The remote Address to connect to.
     * @param channels The number of channels to use.
     * @param data Optional data to associate with the connection.
     * @param timeout The time to wait for a connection.
     * @return A Peer representing the connection.
     * @throws std::runtime_error if the connection fails.
     */
    Peer connect(Address address, size_t channels = 1, uint32 data = 0,
                 uint32 timeout = 5000) {
        m_logger.debug("Connecting to %x:%u", address.host(), address.port());
        ENetPeer* peer;
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            peer = enet_host_connect(m_host, address.get(), channels, data);
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
        }
        flush();

        return Peer(peer);
    }

    /**
     * @brief Flushes any queued packets to the network.
     */
    void flush() {
        m_logger.trace("flushing ENet host");
        std::lock_guard<std::mutex> lock(m_mutex);
        enet_host_flush(m_host);
    }

    /**
     * @brief Broadcasts a packet to all connected peers.
     *
     * This function broadcasts a packet to all peers connected to the host on
     * a specific channel. After the packet is sent, the packet is flagged with
     * `set_dont_free()` to prevent it from being freed immediately.
     *
     * @param packet The packet to be broadcasted.
     * @param channel The channel on which the packet will be broadcast.
     *                Defaults to 0.
     */
    void broadcast(Packet& packet, uint8 channel = 0) {
        m_logger.trace("broadcasting %lu bytes from ENet host",
                       packet.length());
        std::lock_guard<std::mutex> lock(m_mutex);
        enet_host_broadcast(m_host, channel, packet.get());
        packet.release_ownership();
    }

    /**
     * @brief Limits the incoming and outgoing bandwidth for the host.
     *
     * This function sets the maximum incoming and outgoing bandwidth for the
     * host, which can help manage network traffic. Setting either value to 0
     * disables bandwidth limitation for that direction.
     *
     * @param incoming_bandwidth The maximum incoming bandwidth in bytes per
     * second.
     * @param outgoing_bandwidth The maximum outgoing bandwidth in bytes per
     * second.
     */
    void bandwidth_limit(uint32 incoming_bandwith, uint32 outgoing_bandwidth) {
        std::lock_guard<std::mutex> lock(m_mutex);
        enet_host_bandwidth_limit(m_host, incoming_bandwith,
                                  outgoing_bandwidth);
    }

    /**
     * @brief Throttles the bandwidth used by the host.
     *
     * This function performs bandwidth throttling by adjusting the packet
     * delivery schedule based on the network conditions. It is useful for
     * managing network congestion.
     */
    void bandwidth_throttle() {
        std::lock_guard<std::mutex> lock(m_mutex);
        enet_host_bandwidth_throttle(m_host);
    }

    /**
     * @brief Sets the maximum number of channels for the host.
     *
     * This function limits the number of channels that peers can use to
     * communicate with the host. Any peers attempting to communicate on
     * channels higher than this limit will be restricted.
     *
     * @param channel_limit The maximum number of channels.
     */
    void channel_limit(size_t channel_limit) {
        std::lock_guard<std::mutex> lock(m_mutex);
        enet_host_channel_limit(m_host, channel_limit);
    }

    /**
     * @brief Retrieves the underlying ENetHost pointer.
     *
     * This function returns a pointer to the underlying ENetHost, allowing
     * direct access to the ENetHost structure for more advanced operations.
     *
     * @return A pointer to the ENetHost structure.
     */
    ENetHost* get() { return m_host; }

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

/**
 * @brief Initializes the ENet library.
 *
 * This function initializes the ENet networking library. It throws a
 * `std::runtime_error` if the initialization fails. The function also
 * registers `enet_deinitialize` to be called automatically when the
 * program exits, ensuring that resources are cleaned up properly.
 *
 * @throws std::runtime_error If ENet initialization fails.
 *
 * @note This function should be called before any ENet operations are
 * performed.
 */
static inline void initialize() {
    if (enet_initialize() != 0) {
        throw std::runtime_error("An error occurred while initializing ENet.");
    }
    atexit(enet_deinitialize);
}

} // namespace enetcpp

#endif // _ENETCPP_ENETCPP_HPP_