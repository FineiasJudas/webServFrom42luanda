#!/usr/bin/python3
import sys
import os
import urllib.parse

length = int(os.environ.get("CONTENT_LENGTH", 0))
ctype  = os.environ.get("CONTENT_TYPE", "")

body = sys.stdin.read(length)

sys.stdout.write("Status: 200 OK\r\n")
sys.stdout.write("Content-Type: text/html\r\n")
sys.stdout.write("\r\n")

sys.stdout.write("<h1>POST recebido!</h1>")
sys.stdout.write("<p><b>CONTENT_TYPE:</b> " + ctype + "</p>")
sys.stdout.write("<p><b>RAW BODY:</b> " + body + "</p>")

# Se for form-urlencoded, decodificar:
if "application/x-www-form-urlencoded" in ctype:
    data = urllib.parse.parse_qs(body)
    sys.stdout.write("<h2>Campos:</h2>")
    for key, val in data.items():
        sys.stdout.write(f"<p>{key}: {val}</p>")
