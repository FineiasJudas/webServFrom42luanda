#!/usr/bin/python3
import sys
import os

length = int(os.environ.get("CONTENT_LENGTH", 0))
body = sys.stdin.read(length)

print("Status: 200 OK")
print("Content-Type: text/html")
print()
print(f"<h1>Recebi via POST: {body}</h1>")