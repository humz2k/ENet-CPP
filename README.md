# ENetCPP

ENetCPP is a header only C++ wrapper around the [ENet](https://github.com/lsalzman/enet) networking library.

# Building
Either just copy and paste `include/enetcpp/enetcpp.hpp` somewhere in your project, or use the included `CMakeLists.txt`.

# Usage

See `include/enetcpp/enetcpp.hpp` for documentation, but here is a simple example.

## Server

```c++
#include <enetcpp/enetcpp.hpp>
#include <iostream>

class MyServer : public enetcpp::Host{
  public:
    using enetcpp::Host::Host;

    void on_event(enetcpp::EventConnect& event) override{
        std::cout << "received connect event" << std::endl;
    }

    void on_event(enetcpp::EventDisconnect& event) override{
        std::cout << "received disconnect event" << std::endl;
    }

    void on_event(enetcpp::EventReceive& event) override{
        std::cout << "received receive event" << std::endl;
    }
};

int main(){
    MyServer server(enetcpp::Address("127.0.0.1",12345),32);

    while (true){
        server.service(100);
        server.flush();
    }
    return 0;
}
```

## Client
```c++
#include <enetcpp/enetcpp.hpp>
#include <iostream>

class MyClient : public enetcpp::Host{
  public:
    using enetcpp::Host::Host;

    void on_event(enetcpp::EventConnect& event) override{
        std::cout << "received connect event" << std::endl;
    }

    void on_event(enetcpp::EventDisconnect& event) override{
        std::cout << "received disconnect event" << std::endl;
    }

    void on_event(enetcpp::EventReceive& event) override{
        std::cout << "received receive event" << std::endl;
    }
};

int main(){
    MyServer client(1);
    enetcpp::Peer server = client.connect(enetcpp::Address("127.0.0.1",12345));

    std::string data = "ping";
    enetcpp::Packet packet(data.c_str(),data.size());
    server.send(packet);

    while (true){
        client.service(100);
        client.flush();
    }
    return 0;
}
```