#include "pingpong.hpp"
#include <enetcpp/enetcpp.hpp>
#include <iostream>
#include <string>

int main() {
    enetcpp::initialize();

    PingPong host(enetcpp::Address("127.0.0.1", 12345), 32);
    host.set_count(10);

    while (true) {
        host.service(100);
        host.flush();
    }
    return 0;
}