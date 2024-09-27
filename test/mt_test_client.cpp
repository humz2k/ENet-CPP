#include "enetcpp/enetcpp-mt.hpp"
#include <iostream>

class MyClient : public enetcpp::Host {
  public:
    using enetcpp::Host::Host;
    void on_event(enetcpp::EventReceive& event) override {
        const char* message = (const char*)event.packet().data();
        std::cout << "recv: " << message << std::endl;
    }
};

int main() {
    enetcpp::initialize();
    MyClient client(1);
    client.logger().set_loglevel(enetcpp::LogLevel::NONE);
    auto connection = client.connect(enetcpp::Address("127.0.0.1", 12345));

    while (true) {
        std::string data;
        std::cin >> data;
        if (data == "quit")
            break;
        enetcpp::Packet packet(data.c_str(), data.size());
        connection.send(packet);
        client.flush();
        client.service(500);
    }
    connection.disconnect();
    client.flush();
    return 0;
}