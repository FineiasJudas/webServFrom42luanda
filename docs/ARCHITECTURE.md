webserv/                     # root do projecto
├─ Makefile
├─ README.md
├─ LICENSE
├─ conf/
│  ├─ default.conf
│  └─ sites/                 # configs por "server" (opcional)
│     ├─ site1.conf
│     └─ site2.conf
├─ src/
│  ├─ main.cpp
│  ├─ core/                  # Membro 1 (Core & I/O)
│  │  ├─ Server.hpp
│  │  ├─ Server.cpp
│  │  ├─ Poller.hpp
│  │  ├─ PollerEpoll.cpp
│  │  ├─ Connection.hpp
│  │  ├─ Connection.cpp
│  │  ├─ Buffer.hpp
│  │  └─ Buffer.cpp
│  ├─ config/                # Classe Config + parser (Membro 2 colaborará aqui)
│  │  ├─ Config.hpp
│  │  ├─ Config.cpp
│  │  └─ ConfigParser.hpp
│  ├─ http/                  # Membro 2 (HTTP & Config)
│  │  ├─ HttpParser.hpp
│  │  ├─ HttpParser.cpp
│  │  ├─ Request.hpp
│  │  ├─ Response.hpp
│  │  └─ Router.hpp
│  ├─ cgi/                   # Membro 3 (CGI & Features)
│  │  ├─ CgiHandler.hpp
│  │  ├─ CgiHandler.cpp
│  │  └─ StaticFiles.hpp
│  ├─ utils/
│  │  ├─ Logger.hpp
│  │  └─ Utils.hpp
│  └─ tests/                 # testes unitários / scripts
│     ├─ core_tests/
│     ├─ conf_tests/
│     └─ http_tests/
├─ examples/
│  ├─ www/                   # pasta estática de exemplo (site root)
│  └─ cgi-bin/
└─ docs/
   ├─ ARCHITECTURE.md
   ├─ CORE_README.md
   └─ CONFIG_SPEC.md
