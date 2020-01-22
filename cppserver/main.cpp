#include <iostream>

#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>

#include "deadfish_generated.h"

typedef websocketpp::server<websocketpp::config::asio> server;

void on_message(websocketpp::connection_hdl hdl, server::message_ptr msg) {
    std::cout << msg->get_payload() << std::endl;
    const auto payload = msg->get_payload();
    const auto parsed = flatbuffers::GetRoot<DeadFish::CommandMove>(payload.c_str());
    std::cout << parsed->target()->x() << ", " << parsed->target()->y() << std::endl;
}

int main() {
    server print_server;

    print_server.set_message_handler(&on_message);

    print_server.init_asio();
    print_server.listen(63987);
    print_server.start_accept();

    print_server.run();
}