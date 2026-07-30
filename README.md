*This project has been created as part of the 42 curriculum by klaayoun, aberkass, hfegrach.*

## Description

Webserv is an HTTP/1.1 server written in C++98. The goal is to implement a non-blocking web server that can be configured like NGINX and tested with a real browser.

The server reads a configuration file, listens on one or more ports, and handles client connections through a single `epoll` event loop. It serves static files and supports request body uploads, CGI scripts, redirections, directory listing, and custom error pages.

## Features

- Non-blocking I/O with a single `epoll` instance (sockets and CGI pipes)
- Multiple `server` / `listen` directives (multiple ports)
- HTTP methods: `GET`, `POST`, `DELETE`
- Static file serving with per-location `root` and `index`
- File uploads via `upload_store`
- CGI execution (`fork` / `execve`) by file extension (`cgi_pass`)
- Directory listing (`autoindex`)
- HTTP redirections (`return`)
- Method restrictions per location
- Custom and default error pages
- Client body size limit (`client_max_body_size`)
- Chunked transfer decoding for request bodies

## Instructions

**Requirements:** Linux, a C++98 compiler (`c++`).

### Build

```bash
make              # debug build (default)
make release=1    # optimized release build
make sanitize=1   # build with AddressSanitizer
make clean        # remove object files
make fclean       # remove objects and binaries
make re           # full rebuild
```

The binary is written to `build/debug/webserv` (or `build/release/webserv`).

### Run

```bash
./build/debug/webserv [configuration_file]
```

If no configuration file is given, `nginx/webserv.conf` is used.

```bash
make run                          # runs with nginx/webserv.conf
make run ARGS=path/to/file.conf   # custom config
```

Open `http://localhost:8080` in a browser (see the sample config for ports and routes).

## Configuration

The config syntax is inspired by NGINX. Useful directives:

| Directive | Role |
| --- | --- |
| `listen` | Interface/port to bind |
| `root` | Document root for a location |
| `index` | Default file for directories |
| `methods` | Allowed HTTP methods |
| `client_max_body_size` | Max request body size |
| `error_page` | Custom error page path |
| `autoindex` | Directory listing on/off |
| `return` | Redirect to another URI |
| `upload_store` | Upload destination |
| `cgi_pass` | Extension → interpreter |

A full example lives in `nginx/webserv.conf`, with static content under `nginx/www/`.

## Project structure

```
src/
  server/   Event loop, clients, routing, HTTP methods
  http/     Request parsing (FSM), headers, body handling
  cgi/      CGI process management and response parsing
  parser/   Configuration scanner and parser
  config/   Server / location / shared config model
  lib/      Arena allocator and small utilities
  logger/   Logging helpers
nginx/
  webserv.conf   Sample configuration
  www/           Sample website and CGI scripts
```

## Resources

- [RFC 2616](https://www.rfc-editor.org/rfc/rfc2616) — HTTP/1.1
- [RFC 7230](https://www.rfc-editor.org/rfc/rfc7230) — HTTP/1.1 Message Syntax and Routing
- [RFC 3875](https://www.rfc-editor.org/rfc/rfc3875) — The Common Gateway Interface (CGI/1.1)
- Lewis Van Winkle — *Hands-On Network Programming with C*

AI was used to research edge cases and common HTTP/server behaviors during development and debugging.
