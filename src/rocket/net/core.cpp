#include "rocket/net/core.hpp"
#include "rocket/macros.hpp"
#include <cstring>
#include <string>
#include <vector>

#include <boost/asio.hpp>

namespace asio = boost::asio;

namespace rocket::net {
    void init() {}

    bool check_connection_status() {
        asio::io_context io;

        asio::ip::tcp::resolver resolver(io);
        boost::system::error_code ec;

        // resolve host
        auto endpoints = resolver.resolve("example.com", "80", ec);
        if (ec) {
            return false;
        }

        asio::ip::tcp::socket socket(io);

        // connect
        asio::connect(socket, endpoints, ec);
        if (ec) {
            return false;
        }

        // send minimal HEAD request
        std::string req = "HEAD / HTTP/1.1\r\nHost: example.com\r\n\r\n";
        asio::write(socket, asio::buffer(req), ec);
        if (ec) {
            return false;
        }

        // read first bytes
        char buf[512];
        size_t n = socket.read_some(asio::buffer(buf), ec);

        if (!ec && n > 0) {
            return true;
        } else {
            return false;
        }
    }
}
