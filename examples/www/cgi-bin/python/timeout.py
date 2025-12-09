#!/usr/bin/python3
import time
import sys

time.sleep(10)
sys.stdout.write("Status: 200 OK\r\n")
sys.stdout.write("Content-Type: text/html\r\n")
sys.stdout.write("\r\n")
sys.stdout.write("<h1>Never Prints!</h1>")
