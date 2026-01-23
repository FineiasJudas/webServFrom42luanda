NAME = webserv

CXX = c++

CXXFLAGS = -Wall -Wextra -Werror -std=c++98 -MMD -MP

INCLUDES = -Iincludes

RM = rm -f

SRCS = \
    src/main.cpp \
    src/core/Signal.cpp \
    src/core/Buffer.cpp \
    src/core/Connection.cpp \
    src/core/PollerEpoll.cpp \
    src/core/MasterServer.cpp \
    src/http/HttpParser.cpp \
    src/http/Router.cpp \
    src/http/Response.cpp \
    src/session/SessionManager.cpp \
    src/config/ConfigParser.cpp \
    src/cgi/CgiHandler.cpp \
    src/utils/Logger.cpp

OBJS = $(SRCS:.cpp=.o)

DEPS = $(OBJS:.o=.d)

all: $(NAME)

$(NAME): $(OBJS)
	@echo "Construindo..."
	$(CXX) $(CXXFLAGS) -o $(NAME) $(OBJS)
	@echo "Construção completa..."

# incluir arquivos hpp (.d) gerados automaticamente
-include $(DEPS)

clean:
	@echo "Limpando arquivos objeto..."
	$(RM) $(OBJS) $(DEPS)

fclean: clean
	@echo "Limpando executável..."
	$(RM) $(NAME)

re: fclean all

.PHONY: all clean fclean re
