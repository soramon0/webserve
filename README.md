# WebServe

Webserve is a custom HTTP/1.1 server implemented in **C++98**. The project focuses
on understanding the intricacies of the HTTP protocol and implementing a robust,
non-blocking network architecture using I/O multiplexing.

---

## 🚀 Main Features

- **HTTP/1.1 Prototype:** Supports essential HTTP functionality for data
  communication on the World Wide Web.
- **I/O Multiplexing:** Uses exactly **one** event-driven multiplexer
  (`poll()` / `epoll()` / `kqueue()`) for all I/O operations between clients and the server.
- **Non-Blocking Architecture:** All I/O that can wait for data (sockets, pipes)
  is non-blocking and driven by the event loop.
- **Static Website Support:** Capable of serving a fully static website.
- **HTTP Methods:** Implements mandatory `GET`, `POST`, and `DELETE` methods.
- **CGI Execution:** Supports execution of CGI scripts (e.g., PHP, Python)
  based on file extensions.
- **File Uploads:** Enables clients to upload files to a designated storage location.
- **Custom Configuration:** Driven by an NGINX-inspired configuration file to
  define ports, routes, limits, and CGI behavior.

---

## 🛠 Build & Run

### Requirements

- C++98 compatible compiler (e.g. `c++`, `clang++`)
- POSIX system (Linux / macOS)
- `make`

### Build

```bash
make        # builds the webserve binary
make re     # rebuilds from scratch
make clean  # removes object files
make fclean # removes objects + binary
make run    # run server
```

### Run

```bash
./webserve configs/example.conf
```

If no configuration file is provided, the server can optionally fall back to a
default configuration (depending on your implementation).

---

## ⚙️ Server Architecture & Flow

The server operates through a strict life-cycle: parsing configuration,
setting up sockets, and entering the main event loop.

### 1. Initialization & Configuration

The server initializes its internal state and parses the configuration file to
define server blocks and routes.

```cpp
// Initialize the Server
WebServer &server = WebServer::getInstance();

// Parse config to set ports, error pages, and body limits
Config config = Parser::parse(argv[1]);

// Setup listening sockets for each interface:port pair
for (size_t i = 0; i < config.server_blocks.size(); ++i) {
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    // Use fcntl to set non-blocking (Required for MacOS)
    fcntl(fd, F_SETFL, O_NONBLOCK);
    bind(fd, ...);
    listen(fd, ...);
    server.addToMultiplexer(fd, POLLIN); // Register for readiness
}
```

### 2. The Non-Blocking Event Loop

All client and server communication is driven by a single multiplexing call.

```cpp
while (server.isRunning()) {
    // Single poll() monitors reading and writing simultaneously
    int ready = poll(fds, nfds, -1);

    for (int i = 0; i < nfds; ++i) {
        if (fds[i].revents & POLLIN) {
            if (is_listening_socket(fds[i].fd)) {
                acceptNewClient(fds[i].fd); // accept() into a non-blocking socket
            } else {
                handleRead(fds[i].fd); // Perform read() only after readiness
            }
        }
        if (fds[i].revents & POLLOUT) {
            handleWrite(fds[i].fd); // Perform write() only after readiness
        }
    }
}
```

---

## 📜 Configuration Overview

The server is configured using an NGINX-inspired configuration file. You can:

- **Define listening interfaces and ports**
- **Set default error pages**
- **Limit maximum client body size**
- **Configure per-route rules**, including:
  - Accepted HTTP methods (`GET`, `POST`, `DELETE`)
  - HTTP redirections
  - `root` directory for URL mapping
  - `index` file served when a directory is requested
  - `autoindex` (directory listing) on/off
  - Upload directory for file uploads
  - CGI execution based on file extension

---

## 📁 Minimal Example Configuration

```conf
server {
    listen 127.0.0.1:8080;

    server_name localhost;

    # Where static files are served from
    root ./www;

    # Default error pages
    error_page 404 /errors/404.html;
    error_page 500 /errors/500.html;

    # Maximum allowed size for client request bodies (in bytes)
    client_max_body_size 1000000;

    location / {
        methods GET;
        index index.html;
        autoindex off;
    }

    location /upload {
        methods POST;
        upload_store ./uploads;
    }

    location /cgi-bin {
        methods GET POST;
        cgi_pass .py /usr/bin/python3;
        root ./cgi-bin;
    }
}
```

- **`listen`**: interface and port.
- **`root`**: base directory for static files.
- **`location`** blocks: per-URL rules (methods, index, autoindex, upload dir, CGI).

---

## 🌐 Multiple Server Ports Example

The server can listen on multiple ports and serve different content per `server` block.

```conf
# Development server on port 8080
server {
    listen 127.0.0.1:8080;
    server_name localhost;

    root ./www/dev;

    location / {
        methods GET;
        index index.html;
    }
}

# Production-like server on port 8081
server {
    listen 127.0.0.1:8081;
    server_name localhost;

    root ./www/prod;

    location / {
        methods GET;
        index index.html;
    }

    location /api {
        methods GET POST;
        root ./www/api;
    }
}

# Testing server on port 8082
server {
    listen 127.0.0.1:8082;
    server_name localhost;

    root ./www/test;

    location / {
        methods GET;
        autoindex on;
    }

    location /upload {
        methods POST;
        upload_store ./test_uploads;
    }
}
```

With this configuration, you can hit each port separately:

```bash
curl http://127.0.0.1:8080/   # serves from ./www/dev
curl http://127.0.0.1:8081/   # serves from ./www/prod
curl http://127.0.0.1:8082/   # serves from ./www/test
```

This demonstrates the requirement from the subject to listen on multiple ports and deliver different content.

---

## 📡 Communication Pathways

### Case 1: Normal (Static) Response

Standard request for static resources like HTML or images.

- Read Readiness: Multiplexer detects data; server reads the HTTP request.
- Request Parsing: Server un-chunks data if necessary and identifies the method.
- Route Mapping: Maps the URL to a physical directory based on config rules.
- Disk I/O: Server reads the file (regular disk files are exempt from poll monitoring).
- Response: Server generates accurate status codes and serves the content.

### Case 2: Passing to CGI

Dynamic execution of scripts for processing client data.

- CGI Identification: Server matches file extension to a CGI handler.
- Forking: A child process is created using `fork()` (authorized only for CGI).
- Piping: Full request and arguments are passed to the CGI via environment variables.
- Non-Blocking Output: The CGI output pipe is added to the same event loop to read the result.
- Data Delivery: Server captures CGI output (handling EOF as end of data) and sends it to the client.

---

## ✅ Implemented Features (Subject Checklist)

- **Non-blocking I/O**
  - All sockets and pipes are non-blocking.
  - A **single** I/O multiplexing call (`poll`/`epoll`/`kqueue`) drives all client and server I/O.

- **HTTP Methods**
  - `GET` — static files, directory handling (with optional autoindex).
  - `POST` — request body handling and file uploads.
  - `DELETE` — deletion of resources when allowed by configuration.

- **Static Website Support**
  - Serves regular files from configured `root` directories.
  - Supports per-route `index` files and custom error pages.

- **Uploads**
  - File uploads are accepted on configured routes and stored under `upload_store`.

- **CGI**
  - At least one CGI interpreter supported (e.g. Python, PHP) based on file extension.
  - Full request information is passed via environment variables as per CGI spec.

- **Multiple Ports**
  - Multiple `server` blocks / `listen` directives can be defined to serve different content on different ports.

---

## 🔍 Testing the Server

Assuming the server listens on `127.0.0.1:8080`:

### Basic GET (static file)

```bash
curl -v http://127.0.0.1:8080/
curl -v http://127.0.0.1:8080/index.html
```

### File Upload (POST)

```bash
curl -v -F "file=@README.md" http://127.0.0.1:8080/upload
```

### DELETE Resource

```bash
curl -v -X DELETE http://127.0.0.1:8080/upload/README.md
```

### Raw HTTP with netcat

```bash
printf 'GET / HTTP/1.1\r\nHost: localhost\r\n\r\n' | nc 127.0.0.1 8080
```

You can also open a browser and navigate to:

```text
http://127.0.0.1:8080/
```

---

## 🧪 CGI Usage Example

### 1. Configuration Snippet

```conf
location /cgi-bin {
    methods GET POST;
    root ./cgi-bin;
    cgi_pass .py /usr/bin/python3;
}
```

### 2. Example CGI Script (`hello.py`)

```python
#!/usr/bin/env python3
import os

print("Content-Type: text/html\r\n")
print("<html><body>")
print("<h1>Hello from CGI!</h1>")
print(f"<p>REQUEST_METHOD = {os.environ.get('REQUEST_METHOD')}</p>")
print("</body></html>")
```

Make it executable:

```bash
chmod +x cgi-bin/hello.py
```

### 3. Call the CGI

```bash
curl -v http://127.0.0.1:8080/cgi-bin/hello.py
```

---

## 📚 Resources

- [Transmission Control Protocol (TCP) RFC](https://beej.us/guide/bgnet/html/split-wide/index.html)
- [User Datagram Protocol (UDP) RFC](https://datatracker.ietf.org/doc/html/rfc768)
- [Hypertext Transfer Protocol (HTTP) RFC](https://datatracker.ietf.org/doc/html/rfc2616)
- [The Common Gateway Interface (CGI) RFC](https://datatracker.ietf.org/doc/html/rfc3875)
- [Introduction to event-based programming](https://aiven.io/blog/introduction-to-event-based-programming)
- [Beej's Guide to Network Programming](https://beej.us/guide/bgnet/html/split-wide/index.html)
- [Self-Pipe Trick](https://www.sitepoint.com/the-self-pipe-trick-explained/)
- Hands-On Network Programming in C - Book
