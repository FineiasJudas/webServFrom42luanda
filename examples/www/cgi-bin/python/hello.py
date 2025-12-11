#!/usr/bin/python3

import sys

sys.stdout.write("Status: 200 OK\r\n")
sys.stdout.write("Content-Type: text/html\r\n")
sys.stdout.write("\r\n")
sys.stdout.write("<h1>Hello from Python CGI!</h1>")
