#ifndef _PINGPONG_HPP_
#define _PINGPONG_HPP_

#include <enetcpp/enetcpp.hpp>
#include <iostream>

class PingPong : public enetcpp::Host {
  private:
    int m_count = 10;

  public:
    using enetcpp::Host::Host;

    void set_count(int count) { m_count = count; }

    int count() { return m_count; }

    void on_event(enetcpp::EventReceive& event) {
        std::string data((char*)event.packet().data(), event.packet().length());
        std::cout << data << std::endl;
        enetcpp::Packet packet(data.data(), data.size());
        event.peer().send(packet);
        m_count--;
    }
};

#endif // _PINGPONG_HPP_