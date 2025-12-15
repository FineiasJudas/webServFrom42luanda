webserv/                         # root do projecto
├─ Makefile
├─ README.md
├─ LICENSE
├─ conf/
│  └─ default.conf
├─ src/
│  ├─ main.cpp
│  ├─ core/                      # Membro 1 (Core & I/O)
│  │  ├─ MasterServer.hpp
│  │  ├─ MasterServer.cpp
│  │  ├─ Poller.hpp
│  │  ├─ PollerEpoll.cpp
│  │  ├─ Connection.hpp
│  │  ├─ Connection.hpp
│  │  ├─ Signal.hpp
│  │  ├─ Signal.cpp
│  │  ├─ Buffer.hpp
│  │  └─ Buffer.cpp
│  ├─ exceptions/
│  │  └─ WebServException.hpp
│  ├─ config/                    # Classe Config + parser (Membro 2 colaborará aqui)
│  │  ├─ Config.hpp
│  │  ├─ Config.cpp
│  │  └─ ConfigParser.hpp
│  ├─ http/                      # Membro 2 (HTTP & Config)
│  │  ├─ HttpParser.hpp
│  │  ├─ HttpParser.cpp
│  │  ├─ Request.hpp
│  │  ├─ Response.hpp
│  │  └─ Router.hpp
│  ├─ session/
│  │  ├─ SessionManager.hpp
│  │  └─ SessionManager.cpp
│  ├─ cgi/                       # Membro 3 (CGI & Features)
│  │  ├─ CgiHandler.hpp
│  │  ├─ CgiHandler.cpp
│  │  └─ StaticFiles.hpp
│  └─ utils/
│     ├─ Logger.hpp
│     ├─ Logger.cpp
|     ├─ keyword.hpp
│     └─ Utils.hpp
├─ examples/
│  └─ sites/                     # configs por "server" (opcional)
│     └─ www/    
│        ├─ site1/               # pasta estática de exemplo (site root)
|        |  ├─ index.html
|        |  └─ cgi-bin/
│        └─ site2/
|           ├─ index.html
|           └─ cgi-bin/
└─ docs/
   ├─ ARCHITECTURE.md
   ├─ CORE_README.md
   └─ CONFIG_SPEC.md
