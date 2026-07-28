#!/usr/bin/env python3
import os
import sys

# Read whatever body the server sends via stdin (per CGI spec: EOF-terminated
# unless CONTENT_LENGTH is set, in which case read exactly that many bytes)
content_length = os.environ.get("CONTENT_LENGTH")
if content_length:
    body = sys.stdin.read(int(content_length))
else:
    body = sys.stdin.read()

# CGI response: header block, blank line, then body
print("Content-Type: text/plain")
print()
print("hello from cgi")
print("method: " + os.environ.get("REQUEST_METHOD", "?"))
print("query: " + os.environ.get("QUERY_STRING", "?"))
print("body received: " + repr(body))