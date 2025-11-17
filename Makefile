NAME        = webserv

CXX         = c++

CXXFLAGS    = -Wall -Wextra -Werror -std=c++98

INCLUDES    = -I./includes

RM          = rm -f

SRCS =  src/main.cpp \
        src/core/Buffer.cpp \
        src/core/Connection.cpp \
        src/core/PollerEpoll.cpp \
        src/core/Server.cpp \
        src/core/ListenSocket.cpp \
        src/http/HttpParser.cpp \
        src/http/Router.cpp \
        src/http/Response.cpp \
        src/config/ConfigParser.cpp 

OBJS = $(SRCS:.cpp=.o)

all: $(NAME)

$(NAME): $(OBJS)
	@echo "🔧 Linking $(NAME)..."
	$(CXX) $(CXXFLAGS) $(INCLUDES) -o $(NAME) $(OBJS)
	@echo "✅ Build complete!"

%.o: %.cpp
	@echo "🧩 Compiling $< ..."
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

clean:
	@echo "🧹 Cleaning object files..."
	$(RM) $(OBJS)

fclean: clean
	@echo "🗑  Removing binary..."
	$(RM) $(NAME)

re: fclean all

.PHONY: all clean fclean re