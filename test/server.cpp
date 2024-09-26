#include "pingpong.hpp"
#include <enetcpp/enetcpp.hpp>
#include <iostream>
#include <string>

int main() {
    PingPong host(enetcpp::Address("127.0.0.1", 12345), 32);
    host.set_count(10);

    while (host.count()) {
        host.service(100);
    }

    host.flush();
    return 0;
}