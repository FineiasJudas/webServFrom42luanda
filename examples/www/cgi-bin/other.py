#!/usr/bin/env python3
import os
import sys

print("Content-Type: text/plain")
print("")  # linha em branco obrigatória

print("=== PYTHON CGI TEST ===")
print()

print("REQUEST_METHOD:", os.getenv("REQUEST_METHOD"))
print("QUERY_STRING:", os.getenv("QUERY_STRING"))
print("CONTENT_LENGTH:", os.getenv("CONTENT_LENGTH"))
print("SCRIPT_NAME:", os.getenv("SCRIPT_NAME"))
print("SCRIPT_FILENAME:", os.getenv("SCRIPT_FILENAME"))
print("REQUEST_URI:", os.getenv("REQUEST_URI"))
print("DOCUMENT_ROOT:", os.getenv("DOCUMENT_ROOT"))
print()

# POST data
if os.getenv("REQUEST_METHOD") == "POST":
    try:
        size = int(os.getenv("CONTENT_LENGTH", 0))
    except:
        size = 0

    data = sys.stdin.read(size)
    print("POST DATA:")
    print(data)
