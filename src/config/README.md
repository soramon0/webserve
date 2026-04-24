# Config structure

This configuration structure takes inspiration from the ’server’ section of
the NGINX configuration file.

## Options

- http directive includes shared config between all servers

```nginx
http {
    client_max_body_size 1m;
    access_log /var/log/webserv.log; # optional
    access_log off;
    types {
        text/html                   html;
        text/css                    css;
        application/javascript      js;
        image/png                   png;
        audio/mpeg                  mp3;
    }

    server {...}
    server {...}
}

```

- Define all the interface:port pairs on which your server will listen to.

```nginx
server {
    # listen interface:port
    listen 0.0.0.0:8080;
    listen 127.0.0.1:8000;
    listen 127.0.0.1;
    listen 8000;
    listen *:8000;
    listen localhost:8000;
}

server {
    listen 0.0.0.0:9000;
}
```

- Set up default error pages.

```nginx
server {
    # This causes an internal redirect to the specified uri with the client request
    # method changed to "GET" (for all methods other than "GET" and "HEAD").
    # Omitting this config option would cause the server hard coded defaults.
    error_page 404 /usr/share/nginx/web/404.html;
    error_page 500 502 503 504 /usr/share/nginx/web/50x.html;
}
```

- Set the maximum allowed size for client request bodies.

```nginx
http {
    # Maximum allowed size for client request bodies (in bytes)
    # defaults to 1m. If the size in a request exceeds the configured value,
    # the 413 (Request Entity Too Large) error is returned.
    client_max_body_size 1000000;
}
```

- Configurations on a URL/route

```nginx
http {
    # Directory where the requested file should be located
    # at least one must be defined.
    # Context: http, server, location
    root /tmp/www;

    server {
        location /about {
            # we look for about in /tmp/www/about

            # List of accepted HTTP methods for the route.
            methods GET POST;

            # Redirecting an old page to a new one
            return 301 /new-page;
        }

        location /home {
            # we look for home in /tmp/test/home
            root /tmp/test;

            # List of accepted HTTP methods for the route.
            methods GET;

            # Enabling or disabling directory listing. Generates a simple HTML page
            # listing every file inside that directory.
            autoindex on; # options are on/off.

            # Default file to serve when the requested resource is a directory.
            # server tries files in order. If none match and `autoindex` is not enabled,
            # the server returns a 403 status. If we have a match, the server does an
            # internal redirect. For example, If you request /, and Nginx finds
            # index.html, it internally changes the request to /index.html. This is
            # crucial because it means Nginx will then look for a new location block
            # that matches /index.html to see if there are any specific rules
            # (like caching or headers) for that specific file.
            # Context: http, server, location
            index index.html index.htm index.php;
        }

        location /upload {
            methods POST;

            # if the request is bigger than max body size, the server returns
            # 413 Request Entity Too Large.
            client_max_body_size 20M; # Allow 20MB on this location

            # determines how much of the file is kept in RAM before Nginx starts
            # writing to a temporary file on disk.
            client_body_buffer_size 128k; # Default is usually 8k or 16k

            # determines where the uploaded files for this path would go. The path is
            # relative to the parent root
            # Context: http, server, location
            upload_store uploads;
        }
    }
}

```

- CGI

```nginx
server {
    location /cgi-bin {
        # Working directory for the CGI child follows this route's filesystem mapping:
        # the process runs with a cwd under this root (typically the script's directory)
        # so relative opens/paths in the script resolve as expected. If not defined,
        # then the CGI would use the parent root.
        root ./cgi-bin;

        # `methods` lists which HTTP verbs may target this location as CGI: the server
        # runs the script for GET/POST (etc.), not serve the script file as plain text.
        methods GET POST;

        # A path under this location (e.g. /cgi-bin/whatever.py) is run with the program
        # from cgi_pass for that file extension.
        cgi_pass .py /usr/bin/python3;
        cgi_pass .php /usr/bin/php8.4;
    }
}
```

### Optional

- [return](https://nginx.org/en/docs/http/ngx_http_rewrite_module.html) statement

Context: server, location

```nginx
# Stops processing and returns the specified code to a client. can be used for redirection

# Used to send a direct response body to the client. Server stops looking for files
# and simply hands the "text" to the browser with the status "code" specified.
return code [text];

# This is the standard explicit redirect. It tells the browser, "The status is [code],
# and you can find the content at [URL]."
return code URL;

# This is a shorthand redirect. You provide a URL without a status code, the server
# assumes you want a 302 (Temporary Redirect).
return URL;
```
