import sys, os
body = sys.stdin.read()
print("Content-Type: text/plain\r")
print("\r")
print("METHOD: " + os.environ.get("REQUEST_METHOD", "NOT SET"))
print("You sent: " + body)