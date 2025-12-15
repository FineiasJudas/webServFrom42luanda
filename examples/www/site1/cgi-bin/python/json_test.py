#!/usr/bin/python3
import sys
import os
import json

length = int(os.environ.get("CONTENT_LENGTH", 0))
ctype  = os.environ.get("CONTENT_TYPE", "")

body = sys.stdin.read(length)

sys.stdout.write("Status: 200 OK\r\n")
sys.stdout.write("Content-Type: application/json\r\n")
sys.stdout.write("\r\n")

response = {
    "content_type": ctype,
    "raw_body": body
}

# Se for JSON, tentar parsear
if "application/json" in ctype:
    try:
        response["parsed"] = json.loads(body)
    except Exception as e:
        response["error"] = str(e)

sys.stdout.write(json.dumps(response, indent=4))