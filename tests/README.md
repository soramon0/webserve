# Webserv — Subject Compliance Test Plan

Checklist of tests to verify the implementation matches [webserv subject v24.0](file:///home/sora/Documents/webserv.pdf). Use a browser, `curl`, `telnet`/`nc`, and optionally stock NGINX for behaviour comparison.

**Pass rule:** the server must never crash, hang forever, or become unavailable under normal or stress use. A hang or crash during evaluation is grade 0.

---

## 0. Setup

Reference config (covers every mandatory feature):

| Host:port | Role | Document root |
|-----------|------|----------------|
| `127.0.0.1:8080` | Static site, redirect, method restriction, custom 404 | `./nginx/www` |
| `127.0.0.1:8081` | CGI, upload, DELETE, body limit, autoindex on/off | `./var/www` |
| `127.0.0.1:8082` | Media / alternate site | `./var/www/website3` |

```bash
make
./build/debug/webserv nginx/nginx.conf
# or: make run
```

Useful tools: `curl -v`, `curl -i`, `nc`/`telnet`, browser DevTools, `siege`/`ab`/`wrk`.

---

## Route map (`nginx/nginx.conf`)

### `127.0.0.1:8080` — static

| Path | Feature under test |
|------|--------------------|
| `GET /` | Index (`index.html`) |
| `GET /hello.html` | Static file; POST → `405` |
| `GET /images/` | `autoindex on` |
| `GET /old` | `return 301 /hello.html` |
| `GET /missing.html` | Custom `error_page 404` |

### `127.0.0.1:8081` — dynamic

| Path | Feature under test |
|------|--------------------|
| `GET /` | Index from `./var/www` |
| `GET /listing/` | `autoindex on` |
| `GET /nolist/` | `autoindex off` (no listing) |
| `POST /upload/<name>` | Upload → `./var/www/uploads/` (`client_max_body_size 100kib`) |
| `DELETE /upload/<name>` | DELETE when allowed |
| `POST /small-body` | Tiny limit (`10b`) → `413` |
| `GET /redir` | `return 302 /` |
| `GET/POST /cgi-bin/*.py` | CGI (Python) |
| `/cgi-bin/session/` | Bonus: cookies / sessions |

### `127.0.0.1:8082` — media

| Path | Feature under test |
|------|--------------------|
| `GET /` | Alternate root + CGI index |
| `GET /video` | Large media / autoindex |
| `GET /dire` | Autoindex under `./var/www` |

---

## 1. Build & project rules

| # | Test | Expected |
|---|------|----------|
| 1.1 | `make` / `make all` | Builds `webserv` without errors |
| 1.2 | `make` again with no source changes | No unnecessary relinking |
| 1.3 | `make clean` then `make fclean` | Objects / binary removed |
| 1.4 | `make re` | Full rebuild works |
| 1.5 | Compile with `-Wall -Wextra -Werror` and `-std=c++98` | Compiles (C++98) |
| 1.6 | No Boost / forbidden external libs | Link line and sources clean |
| 1.7 | `./build/debug/webserv nginx/nginx.conf` | Listens on `8080`, `8081`, `8082` |
| 1.8 | Missing / invalid config | Clean error, no crash |
| 1.9 | README at repo root | Meets subject README requirements |

---

## 2. Non-blocking I/O (critical — grade 0 if wrong)

| # | Test | Expected |
|---|------|----------|
| 2.1 | Single multiplexer | Exactly **one** `poll` / `select` / `epoll` / `kqueue` loop for all client↔server I/O (listen included) |
| 2.2 | Read + write monitored together | Multiplexer watches both directions |
| 2.3 | No `read`/`recv`/`write`/`send` on sockets/pipes without readiness | Only after poll (or equiv.) reports ready |
| 2.4 | Sockets / CGI pipes non-blocking | `fcntl(..., O_NONBLOCK)`; macOS: only `F_SETFL`, `O_NONBLOCK`, `FD_CLOEXEC` |
| 2.5 | No `errno`-driven retry after read/write | Do not branch on `errno` after read/write to “fix” behaviour |
| 2.6 | Disk files exempt | Regular file I/O need not go through poll |
| 2.7 | Client disconnect mid-request / mid-response | Server cleans up; other clients still served |
| 2.8 | Slow client (partial request, then pause) | Does not block other connections |
| 2.9 | No `execve` of another web server | Own HTTP stack only |
| 2.10 | `fork` only for CGI | No fork for general request handling |

```bash
# Terminal A: hold a connection open without finishing the request
printf 'GET / HTTP/1.1\r\nHost: localhost\r\n' | nc 127.0.0.1 8080

# Terminal B: other ports / clients must still succeed quickly
curl -s -o /dev/null -w '%{http_code}\n' http://127.0.0.1:8080/
curl -s -o /dev/null -w '%{http_code}\n' http://127.0.0.1:8081/
```

---

## 3. Configuration file

| # | Feature | How to test | Expected |
|---|---------|-------------|----------|
| 3.1 | Multiple `listen` | Curl `8080`, `8081`, `8082` | Different content per port |
| 3.2 | Default error pages | Request missing URL on a route without custom page | Built-in error + correct status |
| 3.3 | Custom `error_page` | `curl -i http://127.0.0.1:8080/missing.html` | Custom 404 body, status 404 |
| 3.4 | `client_max_body_size` | `curl -i -X POST http://127.0.0.1:8081/small-body --data 'too-big-body'` | `413`; server stays up |
| 3.5 | Route methods | `curl -i -X POST http://127.0.0.1:8080/hello.html` | `405` |
| 3.6 | HTTP redirection | `curl -i http://127.0.0.1:8080/old` | `301` + `Location: .../hello.html` |
| 3.7 | `root` mapping | Compare `8080` vs `8081` vs `8082` roots | Distinct files / sites |
| 3.8 | Directory listing on | `curl -i http://127.0.0.1:8080/images/` or `http://127.0.0.1:8081/listing/` | Listing HTML |
| 3.9 | Directory listing off | `curl -i http://127.0.0.1:8081/nolist/` | Error, not listing |
| 3.10 | Default index | `curl -i http://127.0.0.1:8080/` | Serves `index.html` |
| 3.11 | Upload store | POST to `http://127.0.0.1:8081/upload/test.txt` | File under `./var/www/uploads/` |
| 3.12 | CGI by extension | `http://127.0.0.1:8081/cgi-bin/query.py` | Script runs |

---

## 4. HTTP methods

### 4.1 GET

```bash
curl -i http://127.0.0.1:8080/
curl -i http://127.0.0.1:8080/missing.html
curl -i http://127.0.0.1:8080/images/
curl -i http://127.0.0.1:8081/listing/
curl -i http://127.0.0.1:8081/nolist/
```

| # | Case | Expected |
|---|------|----------|
| 4.1.1 | Existing file | `200`, correct body |
| 4.1.2 | Missing file | `404` (+ custom page on `:8080`) |
| 4.1.3 | Directory + index | Serves index |
| 4.1.4 | Directory + autoindex | Listing or forbid per config |
| 4.1.5 | Static site in browser | `http://127.0.0.1:8080/` loads HTML/CSS/images |

### 4.2 POST

```bash
curl -i -X POST http://127.0.0.1:8081/upload/hello.txt \
  -H 'Content-Type: text/plain' --data 'hello'

curl -i -X POST http://127.0.0.1:8081/upload/hosts.txt \
  -H 'Content-Type: text/plain' --data-binary @/etc/hosts

# over limit (100kib on /upload, 10b on /small-body)
curl -i -X POST http://127.0.0.1:8081/small-body --data 'abcdefghijklmnop'
```

| # | Case | Expected |
|---|------|----------|
| 4.2.1 | Allowed route + body | Success; file stored |
| 4.2.2 | File upload | Appears in `./var/www/uploads/` |
| 4.2.3 | Body over limit | `413`; no crash |
| 4.2.4 | Method not allowed | `POST` to `:8080/hello.html` → `405` |
| 4.2.5 | Empty body | Sensible handling (no hang) |

### 4.3 DELETE

```bash
curl -i -X POST http://127.0.0.1:8081/upload/tmp.txt --data 'x'
curl -i -X DELETE http://127.0.0.1:8081/upload/tmp.txt
curl -i -X DELETE http://127.0.0.1:8081/upload/does-not-exist.txt
curl -i -X DELETE http://127.0.0.1:8080/index.html   # DELETE forbidden on site A
```

| # | Case | Expected |
|---|------|----------|
| 4.3.1 | Existing deletable resource | Success; file gone from `uploads/` |
| 4.3.2 | Missing resource | Accurate error (`404`) |
| 4.3.3 | DELETE forbidden on route | `405` on `:8080` |

---

## 5. Status codes & headers

| # | Scenario | How | Typical status |
|---|----------|-----|----------------|
| 5.1 | OK | `GET http://127.0.0.1:8080/` | `200` |
| 5.2 | Redirect | `GET http://127.0.0.1:8080/old` | `301` + `Location` |
| 5.3 | Bad request | Malformed request via `nc` | `400` |
| 5.4 | Forbidden | Depends on autoindex/upload rules | `403` |
| 5.5 | Not found | `GET http://127.0.0.1:8080/missing.html` | `404` |
| 5.6 | Method not allowed | `POST http://127.0.0.1:8080/hello.html` | `405` |
| 5.7 | Payload too large | `POST http://127.0.0.1:8081/small-body` | `413` |
| 5.8 | Internal / CGI failure | Break a CGI script temporarily | `500` (or accurate 5xx) |

Also check: status line format, `Content-Length` / chunked encoding, no infinite wait on incomplete requests.

---

## 6. CGI (at least one type required)

```bash
curl -i 'http://127.0.0.1:8081/cgi-bin/query.py?name=webserv'
curl -i -X POST http://127.0.0.1:8081/cgi-bin/upload.py --data 'a=1&b=2'
curl -i http://127.0.0.1:8081/cgi-bin/timeout.py   # must not hang the whole server
curl -i http://127.0.0.1:8081/cgi-bin/cgi.sh
```

| # | Test | Expected |
|---|------|----------|
| 6.1 | GET CGI with query string | `QUERY_STRING` visible; correct output |
| 6.2 | POST CGI with body | Body on stdin / `CONTENT_LENGTH` |
| 6.3 | Chunked request body | Server unchunks; CGI gets plain body then EOF |
| 6.4 | CGI without `Content-Length` | Server reads until EOF |
| 6.5 | Relative paths inside CGI | Correct working directory |
| 6.6 | CGI crash / non-zero exit | Error response; server keeps running |
| 6.7 | Slow / hanging CGI | Other clients on `8080`/`8081` still served |
| 6.8 | Only CGI uses `fork`/`execve` | Confirmed in code review |

---

## 7. Browser compatibility

| # | Test | Expected |
|---|------|----------|
| 7.1 | Open `http://127.0.0.1:8080/` | Pages, CSS, images load |
| 7.2 | Open `http://127.0.0.1:8081/` | Index + CGI links work |
| 7.3 | Open `http://127.0.0.1:8082/` | Media / website3 |
| 7.4 | Upload / forms on `:8081` | Files land in `uploads/` |
| 7.5 | Refresh / multiple tabs across ports | Stable; no crash |

---

## 8. Multiple ports / multiple sites

```bash
curl -i http://127.0.0.1:8080/
curl -i http://127.0.0.1:8081/
curl -i http://127.0.0.1:8082/
```

| # | Test | Expected |
|---|------|----------|
| 8.1 | Three listens in one process | All respond |
| 8.2 | Different roots / content | Distinct sites (`nginx/www` vs `var/www` vs `website3`) |
| 8.3 | One port busy / bind error | Clear failure — no silent crash loop |

---

## 9. Resilience & stress (mandatory)

| # | Test | How | Expected |
|---|------|-----|----------|
| 9.1 | Concurrent clients | `siege -c 50 -t 30s http://127.0.0.1:8080/` | Stays available |
| 9.2 | Rapid connect/disconnect | Script open/close sockets on `8080`/`8081` | Stable |
| 9.3 | Incomplete headers then close | `printf 'GET / HTTP/1.1\r\n' \| nc 127.0.0.1 8080` then kill | Cleanup; others OK |
| 9.4 | Huge headers / many headers | Oversized request | Reject / close; no crash |
| 9.5 | Body > limit under load | Parallel POSTs to `http://127.0.0.1:8081/small-body` | Errors; server lives |
| 9.6 | Mix GET/POST/DELETE/CGI | Traffic across `8080`–`8082` | Correct responses |
| 9.7 | Never hang indefinitely | Slowloris-style drip on `8080` | Timeout/close; loop continues |

```bash
siege -c 100 -t 1m http://127.0.0.1:8080/
# or
for i in $(seq 1 200); do curl -s -o /dev/null http://127.0.0.1:8080/ & done; wait
curl -s -o /dev/null -w '%{http_code}\n' http://127.0.0.1:8081/
```

---

## 10. Telnet / raw HTTP (protocol sanity)

```text
$ telnet 127.0.0.1 8080
GET / HTTP/1.1
Host: 127.0.0.1

```

| # | Test | Expected |
|---|------|----------|
| 10.1 | Valid minimal request | Valid status line + headers + body |
| 10.2 | Unknown method | Accurate error (e.g. `405`/`501`) |
| 10.3 | HTTP/1.0 vs 1.1 quirks | Sensible behaviour |
| 10.4 | Connection close / keep-alive | Matches what you claim to support |

---

## 11. NGINX comparison (recommended)

Compare key behaviours with stock NGINX where the directives overlap (`listen`, `root`, `index`, `autoindex`, `error_page`, `return`, body size). Custom webserv directives (`methods`, `cgi_pass`, `upload_store`) have no direct stock equivalent — validate those against this config’s expected outcomes instead.

Document intentional differences (HTTP version, header order, date format).

---

## 12. Bonus (only if mandatory is solid)

| # | Feature | Test |
|---|---------|------|
| 12.1 | Cookies / sessions | Browser: `http://127.0.0.1:8081/cgi-bin/session/` |
| 12.2 | Multiple CGI types | `:8081` runs `.py` and `.sh` (`cgi.php` if `php-cgi` is installed) |

---

## 13. Defense-day checklist (quick)

- [ ] Started with `nginx/nginx.conf` — `8080`, `8081`, `8082` all live
- [ ] Browser: static site on `http://127.0.0.1:8080/`
- [ ] `curl` GET / POST upload / DELETE on `:8081`
- [ ] CGI: `http://127.0.0.1:8081/cgi-bin/query.py?name=demo`
- [ ] Custom 404 on `:8080/missing.html`; body-limit `413` on `:8081/small-body`
- [ ] Redirect `:8080/old` → `/hello.html`; POST `:8080/hello.html` → `405`
- [ ] Autoindex on `:8081/listing/` vs off `:8081/nolist/`
- [ ] Stress on `:8080` while curling `:8081` still works
- [ ] Can explain single poll loop + non-blocking readiness rules
- [ ] Ready for a small on-the-spot code change

---

## Config matrix ↔ this file

| Directive / feature | Where in `nginx/nginx.conf` |
|---------------------|-----------------------------|
| `listen` ×3 | `127.0.0.1:8080`, `:8081`, `:8082` |
| `error_page` | Site A / B → `error_pages/404.html` |
| `client_max_body_size` | Global `1mib`; `/upload` `100kib`; `/small-body` `10b` |
| `methods` | Site A GET-only; `/upload` POST+GET(+DELETE) |
| `return` / redirect | `/old` on `:8080`; `/redir` on `:8081` |
| `root` | `nginx/www`, `var/www`, `var/www/website3` |
| `autoindex` | `/images/`, `/listing/` on; `/`, `/nolist/` off |
| `index` | `index.html` / `test_website.py` |
| `upload_store` | `uploads` under `./var/www` |
| CGI | `/cgi-bin` on `:8081` (`.py`, `.sh`) |

---

## Out of scope / optional

- Full RFC coverage not required
- Virtual hosts optional
- 42’s small tester optional if your own tests + browser are solid
