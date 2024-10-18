#include <iostream>
#include <string>
#include <thread>
#include <unordered_map>
#include <functional>
#include <asio.hpp>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <chrono>
#include <iomanip>
#include <ctime>

using asio::ip::tcp;
namespace fs = std::filesystem;

class Logger {
public:
    Logger(const std::string& filename)
        : log_file_(filename, std::ios::app)
    {
        if (!log_file_.is_open()) {
            throw std::runtime_error("Failed to open log file");
        }
    }

    void log(const std::string& client_ip, const std::string& request, const std::string& status)
    {
        std::lock_guard<std::mutex> guard(log_mutex_);

        std::string timestamp = getTimestamp();
        std::string log_entry = client_ip + " - - [" + timestamp + "] \"" + request + "\" " + status;

        // Log to console
        std::cout << log_entry << std::endl;

        // Log to file
        log_file_ << log_entry << std::endl;
    }

private:
    std::ofstream log_file_;
    std::mutex log_mutex_;

    std::string getTimestamp()
    {
        auto now = std::chrono::system_clock::now();
        std::time_t now_time = std::chrono::system_clock::to_time_t(now);
        std::tm now_tm = *std::localtime(&now_time);

        std::ostringstream timestamp;
        timestamp << std::put_time(&now_tm, "%d/%b/%Y:%H:%M:%S %z");

        return timestamp.str();
    }
};

class WebServer {
public:
    WebServer(unsigned short port, Logger& logger)
        : acceptor_(io_context_, tcp::endpoint(tcp::v4(), port))
        , logger_(logger)
    {
        startAccept();
    }

    void addRoute(const std::string& route, std::function<std::string()> handler)
    {
        routes_[route] = handler;
    }

    void addFileProviderRoute(const std::string& route_prefix, const std::string& folder_path)
    {
        routes_[route_prefix] = [this, folder_path, route_prefix]() {
            try {
                std::string sanitized_path = sanitizePath(route_prefix, folder_path);
                if (fs::exists(sanitized_path) && fs::is_regular_file(sanitized_path)) {
                    std::ifstream file(sanitized_path, std::ios::binary);
                    std::ostringstream ss;
                    ss << file.rdbuf();
                    return ss.str();
                } else {
                    return "404 File Not Found";
                }
            } catch (const std::runtime_error& e) {
                return "403 Forbidden: " + std::string(e.what());
            }
        };
    }

    void run()
    {
        io_context_.run();
    }

private:
    void startAccept()
    {
        auto socket = std::make_shared<tcp::socket>(io_context_);
        acceptor_.async_accept(*socket, [this, socket](std::error_code ec) {
            if (!ec) {
                std::thread(&WebServer::handleRequest, this, socket).detach();
            }
            startAccept();
        });
    }

    void handleRequest(std::shared_ptr<tcp::socket> socket)
    {
        try {
            char buffer[1024];
            std::error_code error;
            size_t length = socket->read_some(asio::buffer(buffer), error);

            if (!error) {
                std::string request(buffer, length);
                std::string client_ip = socket->remote_endpoint().address().to_string();
                std::string route = parseRoute(request);
                std::string status;

                std::string response;
                if (routes_.find(route) != routes_.end()) {
                    response = "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\n\r\n" + routes_[route]();
                    status = "200 OK";
                } else {
                    response = "HTTP/1.1 404 Not Found\r\nContent-Type: text/plain\r\n\r\nRoute Not Found";
                    status = "404 Not Found";
                }

                asio::write(*socket, asio::buffer(response));

                // Log the request with nginx-style format
                logger_.log(client_ip, "GET " + route, status);
            }
        } catch (std::exception& e) {
            logger_.log("unknown", "error", "500 Internal Server Error");
        }
    }

    std::string parseRoute(const std::string& request)
    {
        size_t start = request.find("GET ") + 4;
        size_t end = request.find(" ", start);
        return request.substr(start, end - start);
    }

    std::string sanitizePath(const std::string& route_prefix, const std::string& base_path)
    {
        // Ensure the request doesn't include ".." to avoid directory traversal
        fs::path requested_path = fs::path(route_prefix).filename();

        // Ensure the path is within the public folder by resolving relative path
        fs::path full_path = fs::canonical(base_path / requested_path);

        // Check if the resolved path starts with the base folder (i.e., within /public)
        if (full_path.string().find(fs::canonical(base_path).string()) == 0) {
            return full_path.string();
        } else {
            throw std::runtime_error("Invalid file request - potential directory traversal attempt.");
        }
    }

    asio::io_context io_context_;
    tcp::acceptor acceptor_;
    std::unordered_map<std::string, std::function<std::string()>> routes_;
    Logger& logger_;
};

int main()
{
    try {
        Logger logger("access.log");

        WebServer server(8080, logger);

        // Define simple routes
        server.addRoute("/", []() {
            return "Welcome to the homepage!";
        });
        server.addRoute("/about", []() {
            return "This is the about page.";
        });

        // Define file provider route for /public
        server.addFileProviderRoute("/public", "./public");

        server.run();
    } catch (std::exception& e) {
        std::cerr << "Exception: " << e.what() << std::endl;
    }

    return 0;
}
