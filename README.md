
# C++ Web Server with Route Management and File Provider

This is a simple multithreaded web server implemented in C++ using the Asio library. It supports basic route management, logging (similar to Nginx access logs), and secure file serving from a designated folder.

## Features

- **Route management**: Define custom routes that respond to specific HTTP requests.
- **File provider**: Serve static files from a folder (`/public` by default) with proper path sanitization to prevent directory traversal attacks.
- **Logging**: Logs HTTP requests in the Nginx-style access log format, with both console and file logging.
- **Multithreading**: Each request is handled in a separate thread to improve performance.

## Dependencies

- **Xmake** build system
- **C++17** or later
- [Asio](https://think-async.com/Asio/) (provided by xmake)

## Getting Started

### 1. Clone the Repository

```bash
git clone <repository-url>
cd <repository-folder>
```

### 2. Build the Project with Xmake

#### Install Xmake

If you don't have Xmake installed, you can install it by following the instructions from the [Xmake website](https://xmake.io/#/).

#### Build the Project

After installing Xmake, you can build the project by running:

```bash
xmake
```

This will compile the project and generate the executable in the `build` directory.

### 4. Run the Server

Once the project is built, run the server:

```bash
xmake run WebServer
```

The server will start listening on port `8080` by default. You can access it via `http://localhost:8080`.

### 5. Accessing Routes

- `/` - Responds with a welcome message: "Welcome to the homepage!"
- `/about` - Responds with "This is the about page."
- `/public/*` - Serves static files located in the `./public` folder. Example: `http://localhost:8080/public/file.txt`.

### 6. Logging

Logs are saved in an `access.log` file in the project root directory. Logs follow the Nginx access log style:

```
<client-ip> - - [timestamp] "GET <route>" <status>
```

Example:

```
127.0.0.1 - - [18/Oct/2024:14:32:45 +0000] "GET /about" 200 OK
```

## Adding Routes

You can define your own custom routes in the `main()` function:

```cpp
server.addRoute("/custom", []() {
    return "This is a custom route!";
});
```

### Serving Files

To serve files securely, define a route prefix (like `/public`) and a folder to serve files from. For example:

```cpp
server.addFileProviderRoute("/public", "./public");
```

This will serve files located in the `./public` folder at the URL `/public/<filename>`.

## License

This project is open-source and licensed under the MIT License.