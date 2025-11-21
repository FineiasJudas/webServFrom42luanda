#!/usr/bin/python3
import sys

body = sys.stdin.read()

sys.stdout.write("Status: 200 OK\r\n")
sys.stdout.write("Content-Type: text/plain\r\n")
sys.stdout.write("\r\n")
sys.stdout.write("Corpo recebido:")
sys.stdout.write(body)