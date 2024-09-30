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
 * @file enetcpp_mt.hpp
 * @brief Multi-threaded extension for ENetCPP networking library.
 *
 * This file provides multi-threaded wrappers around ENetCPP classes to enable
 * asynchronous handling of connections and packets. It introduces the
 * `ConnectionThread` class to manage individual peer connections in their own
 * threads and the `HostMT` class to handle multi-threaded server and client
 * operations.
 *
 * This extension to ENetCPP is designed for applications where each connection
 * requires independent handling of network events like receiving packets or
 * managing disconnections. It ensures that network activities like sending,
 * receiving, and disconnecting are handled efficiently across multiple threads.
 *
 * @note This file extends the functionality of `enetcpp.hpp`.
 */

#ifndef _ENETCPP_ENETCPP_MT_HPP_
#define _ENETCPP_ENETCPP_MT_HPP_

#include "enetcpp.hpp"
#include <condition_variable>
#include <iostream>
#include <mutex>
#include <queue>
#include <thread>

namespace enetcpp {

/**
 * @brief A thread class for managing individual peer connections.
 *
 * The `ConnectionThread` class is responsible for handling a single peer
 * connection in its own thread. It processes incoming packets asynchronously
 * and manages the connection's lifecycle, including queuing and dequeuing
 * packets, sleeping, and waking based on network events.
 *
 * This class is designed to be extended for custom packet handling by
 * overriding the `handle()` method.
 */
class ConnectionThread {
  private:
    Host& m_host;
    Address m_address;
    Peer m_peer;
    std::mutex m_mutex;
    std::condition_variable m_cv;
    std::queue<ENetPacket*> m_packet_queue;
    bool m_should_quit = false;
    std::thread m_thread;
    bool m_launched = false;
    bool m_should_wake = false;

  public:
    /**
     * @brief Constructs a ConnectionThread for a given host, address, and peer.
     * @param host The host managing the connection.
     * @param address The peer's network address.
     * @param peer The peer associated with the connection.
     */
    ConnectionThread(Host& host, Address address, Peer peer)
        : m_host(host), m_address(address), m_peer(peer) {}

    /**
     * @brief Returns a reference to the host managing the connection.
     * @return Reference to the Host object.
     */
    Host& host() { return m_host; }

    /**
     * @brief Wakes the connection thread.
     *
     * This method signals the thread to wake up and process any queued packets
     * or tasks.
     */
    void wake() {
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_should_wake = true;
        }
        m_cv.notify_one();
    }

    /**
     * @brief Puts the connection thread to sleep.
     *
     * The thread will wait until it is woken up by a call to `wake()`.
     */
    void sleep() {
        std::unique_lock lk(m_mutex);
        m_cv.wait(lk, [this] { return m_should_wake; });
    }

    /**
     * @brief Returns a reference to the peer associated with this thread.
     * @return Reference to the Peer object.
     */
    Peer& peer() { return m_peer; }

    /**
     * @brief Returns the address of the peer associated with this thread.
     * @return Reference to the Address object.
     */
    Address& address() { return m_address; }

    /**
     * @brief Launches the connection thread.
     *
     * Starts the thread and begins processing the connection asynchronously.
     * @throws std::runtime_error if the thread has already been launched.
     */
    void launch() {
        m_launched = true;
        m_thread = std::thread(&ConnectionThread::run, this);
    }

    /**
     * @brief Joins the connection thread.
     *
     * Waits for the thread to complete execution.
     * @throws std::runtime_error if the thread has not been launched.
     */
    void join() {
        if (m_launched) {
            m_thread.join();
            return;
        }
        throw std::runtime_error("thread joined but not launched");
    }

    /**
     * @brief Queues a packet to be processed by the connection thread.
     * @param packet Pointer to the ENetPacket to queue.
     */
    void queue_packet(ENetPacket* packet) {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_packet_queue.push(packet);
    }

    /**
     * @brief Dequeues a packet from the queue.
     * @return Pointer to the dequeued ENetPacket, or NULL if the queue is
     * empty.
     */
    ENetPacket* dequeue_packet() {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_packet_queue.size() == 0)
            return NULL;
        auto out = m_packet_queue.front();
        m_packet_queue.pop();
        return out;
    }

    /**
     * @brief Signals the connection thread to quit.
     *
     * This method sets the `m_should_quit` flag, indicating that the thread
     * should stop running after completing its current tasks.
     */
    void quit() {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_should_quit = true;
    }

    /**
     * @brief Checks whether the connection thread should quit.
     * @return `true` if the thread should quit, `false` otherwise.
     */
    bool should_quit() {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_should_quit;
    }

    /**
     * @brief Returns the current size of the packet queue.
     * @return The number of packets in the queue.
     */
    size_t queue_size() {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_packet_queue.size();
    }

    /**
     * @brief Handles a received packet.
     *
     * This method is intended to be overridden in derived classes to provide
     * custom packet handling logic.
     * @param packet The received Packet to handle.
     */
    virtual void handle(Packet& packet) {}

    /**
     * @brief Main loop for the connection thread.
     *
     * This method runs the main loop, where it processes queued packets and
     * waits for network events. It runs until the `should_quit()` flag is set.
     */
    void run() {
        while (true) {
            sleep();
            {
                std::lock_guard<std::mutex> lock(m_mutex);
                m_should_wake = false;
            }
            while (queue_size() > 0) {
                ENetPacket* raw_packet = dequeue_packet();
                if (raw_packet) {
                    Packet packet(raw_packet);
                    this->handle(packet);
                }
            }
            if (should_quit()) {
                return;
            }
        }
    }
};

/**
 * @brief Multi-threaded host class for managing multiple connection threads.
 *
 * The `HostMT` class extends the `Host` class to manage multiple
 * `ConnectionThread` objects, each of which handles a single peer connection in
 * a separate thread. It manages the lifecycle of connections, including
 * connection, disconnection, and packet reception.
 *
 * This class uses a template parameter `ConnectionThread_t` to specify the type
 * of connection thread to use, allowing for custom behavior.
 *
 * @tparam ConnectionThread_t The type of connection thread to use (derived from
 * `ConnectionThread`).
 */
template <typename ConnectionThread_t> class HostMT : public Host {
  private:
    std::thread m_thread;
    bool m_should_quit = false;
    bool m_launched = false;

  public:
    using Host::Host;

    /**
     * @brief Handles a connection event by creating and launching a new
     * connection thread.
     * @param event The connection event.
     */
    void on_event(EventConnect& event) override {
        ConnectionThread_t* new_connection =
            new ConnectionThread_t(*this, event.address(), event.peer());
        event.set_peer_data(new_connection);
        new_connection->launch();
    }

    /**
     * @brief Handles a disconnection event by stopping and joining the
     * connection thread.
     * @param event The disconnection event.
     */
    void on_event(EventDisconnect& event) override {
        ConnectionThread_t* connection = (ConnectionThread_t*)event.peer_data();
        connection->quit();
        connection->wake();
        connection->join();
        delete connection;
    }

    /**
     * @brief Handles a packet reception event by queueing the packet in the
     * corresponding connection thread.
     * @param event The packet reception event.
     */
    void on_event(EventReceive& event) override {
        event.packet().release_ownership();
        ConnectionThread_t* connection = (ConnectionThread_t*)event.peer_data();
        connection->queue_packet(event.packet().get());
        connection->wake();
    }

    /**
     * @brief Runs the main loop for the host, servicing network events.
     *
     * The host processes network events and flushes the network state in this
     * loop. The loop runs until `should_quit()` returns `true`.
     */
    void run() {
        while (!should_quit()) {
            service(10);
            flush();
        }
    }

    /**
     * @brief Signals the host to stop running.
     */
    void quit() {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_should_quit = true;
    }

    /**
     * @brief Checks whether the host should stop running.
     * @return `true` if the host should quit, `false` otherwise.
     */
    bool should_quit() {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_should_quit;
    }

    /**
     * @brief Joins the host's main thread.
     *
     * Waits for the host's main thread to complete execution.
     * @throws std::runtime_error if the server thread has not been launched.
     */
    void join() {
        if (m_launched) {
            m_thread.join();
            return;
        }
        throw std::runtime_error("Server thread joined but not launched");
    }

    /**
     * @brief Launches the host's main thread and starts servicing network
     * events.
     */
    void launch() {
        m_launched = true;
        m_thread = std::thread(&HostMT::run, this);
    }
};

} // namespace enetcpp

#endif // _ENETCPP_ENETCPP_MT_HPP_