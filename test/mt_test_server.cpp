#include "enetcpp/enetcpp-mt.hpp"
#include <iostream>

class MyConnectionThread : public enetcpp::ConnectionThread {
  public:
    using enetcpp::ConnectionThread::ConnectionThread;

    void handle(enetcpp::Packet& packet) override {
        host().logger().info("%x:%d : %s", address().host(), address().port(),
                             (const char*)packet.data());
        std::string data((const char*)packet.data());
        enetcpp::Packet send_packet(data.c_str(), data.size());
        peer().send(send_packet);
    }
};

int main() {
    enetcpp::initialize();
    enetcpp::HostMT<MyConnectionThread> server(
        enetcpp::Address("127.0.0.1", 12345), 32);
    server.launch();
    while (true) {
        std::string command;
        std::cin >> command;
        if (command == "quit") {
            break;
        }
    }
    server.quit();
    server.join();
    return 0;
}