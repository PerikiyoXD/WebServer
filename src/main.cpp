#include <iostream>
#include <string>
#include <unordered_map>
#include <functional>
#include <asio.hpp>

using asio::ip::tcp;

class WebServer {
public:
    WebServer(unsigned short port) : acceptor_(io_context_, tcp::endpoint(tcp::v4(), port)) {
        startAccept();
    }

    void addRoute(const std::string& route, std::function<std::string()> handler) {
        routes_[route] = handler;
    }

    void run() {
        io_context_.run();
    }

private:
    void startAccept() {
        auto socket = std::make_shared<tcp::socket>(io_context_);
        acceptor_.async_accept(*socket, [this, socket](std::error_code ec) {
            if (!ec) {
                std::thread(&WebServer::handleRequest, this, socket).detach();
            }
            startAccept();
        });
    }

    void handleRequest(std::shared_ptr<tcp::socket> socket) {
        try {
            char buffer[1024];
            std::error_code error;
            size_t length = socket->read_some(asio::buffer(buffer), error);

            if (!error) {
                std::string request(buffer, length);
                std::cout << "Received request: " << request << std::endl;

                std::string route = parseRoute(request);

                std::string response;
                if (routes_.find(route) != routes_.end()) {
                    response = "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\n\r\n" + routes_[route]();
                } else {
                    response = "HTTP/1.1 404 Not Found\r\nContent-Type: text/plain\r\n\r\nRoute Not Found";
                }

                asio::write(*socket, asio::buffer(response));
            }
        } catch (std::exception& e) {
            std::cerr << "Exception: " << e.what() << std::endl;
        }
    }

    std::string parseRoute(const std::string& request) {
        size_t start = request.find("GET ") + 4;
        size_t end = request.find(" ", start);
        return request.substr(start, end - start);
    }

    asio::io_context io_context_;
    tcp::acceptor acceptor_;
    std::unordered_map<std::string, std::function<std::string()>> routes_;
};

int main() {
    try {
        WebServer server(8080);

        // Define routes
        server.addRoute("/", []() {
            return "Welcome to the homepage!";
        });
        server.addRoute("/about", []() {
            return "This is the about page.";
        });
        server.addRoute("/hello", []() {
            return "Hello, World!";
        });

        server.run();
    } catch (std::exception& e) {
        std::cerr << "Exception: " << e.what() << std::endl;
    }

    return 0;
}
