#!/usr/bin/env python3
import sys
import os

content_length = int(os.environ.get("CONTENT_LENGTH", 0))
body = sys.stdin.read(content_length) if content_length > 0 else ""

params = {}
if body:
    for part in body.split("&"):
        if "=" in part:
            k, v = part.split("=", 1)
            params[k] = v

name = params.get("name", "")
age = params.get("age", "")

print("Content-Type: text/html")
print()
print(f"Name: {name} <br>")
print(f"Age: {age} <br>")