#include "pingpong.hpp"
#include <enetcpp/enetcpp.hpp>
#include <iostream>
#include <string>

int main() {
    enetcpp::initialize();

    PingPong host(1);
    host.set_count(9);

    auto peer = host.connect(enetcpp::Address("127.0.0.1", 12345));

    std::string data = "ping";
    enetcpp::Packet packet(data.c_str(), data.size());
    peer.send(packet);

    while (host.count()) {
        host.service(100);
    }
    peer.disconnect();
    host.flush();
    return 0;
}