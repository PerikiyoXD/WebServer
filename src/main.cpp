#include <iostream>
#include <string>
#include <thread>
#include <unordered_map>
#include <functional>
#include <asio.hpp>
#include <fstream>

using asio::ip::tcp;

class Logger {
public:
    Logger(const std::string& filename) : log_file_(filename, std::ios::app) {
        if (!log_file_.is_open()) {
            throw std::runtime_error("Failed to open log file");
        }
    }

    void log(const std::string& message) {
        std::lock_guard<std::mutex> guard(log_mutex_);
        // Log to console
        std::cout << message << std::endl;
        // Log to file
        log_file_ << message << std::endl;
    }

private:
    std::ofstream log_file_;
    std::mutex log_mutex_;
};

class WebServer {
public:
    WebServer(unsigned short port, Logger& logger) 
        : acceptor_(io_context_, tcp::endpoint(tcp::v4(), port)), logger_(logger) {
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
                logger_.log("Received request: " + request);

                std::string route = parseRoute(request);
                std::string response;

                if (routes_.find(route) != routes_.end()) {
                    response = "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\n\r\n" + routes_[route]();
                    logger_.log("Route found: " + route);
                } else {
                    response = "HTTP/1.1 404 Not Found\r\nContent-Type: text/plain\r\n\r\nRoute Not Found";
                    logger_.log("Route not found: " + route);
                }

                asio::write(*socket, asio::buffer(response));
            }
        } catch (std::exception& e) {
            logger_.log("Exception: " + std::string(e.what()));
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
    Logger& logger_;
};

int main() {
    try {
        Logger logger("server.log");

        WebServer server(8080, logger);

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
