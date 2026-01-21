#!/usr/bin/python3
import sys
import os
import html

# --- Cabeçalho HTTP ---
print("Status: 200 OK")
print("Content-Type: text/html; charset=UTF-8")
print()

# --- HTML ---
print(""" 
<!DOCTYPE html>
<html lang="pt-br">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>Olá Mundo - Python CGI</title>
<style>
:root {
    --cor-primaria: #4f46e5;
}

body {
    font-family: system-ui, -apple-system, "Segoe UI", Roboto, Arial;
    background: #f4f4f4;
    color: #161616;
    padding: 2rem;
}

h1 {
    color: var(--cor-primaria);
}

.example-box {
    background: #f4f4f4;
    border-left: 4px solid var(--cor-primaria);
    padding: 1rem;
    border-radius: 4px;
    margin: 1rem 0;
    font-family: 'IBM Plex Mono', monospace;
    font-size: 1rem;
    line-height: 1.4;
}
</style>
</head>
<body>

<h1>Olá Mundo! 🐍</h1>

<div class="example-box">
<p>Este é um CGI Python básico que retorna HTML estilizado.</p>
</div>

</body>
</html>
""")
