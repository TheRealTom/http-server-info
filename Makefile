CXX = g++
CXXFLAGS = -Wall -Werror -Wextra -pedantic -std=c++17 -g -fsanitize=address

SRC = main.cpp
OBJ = $(SRC:.cc=.o)
EXEC = hinfosvc

all: $(EXEC)

$(EXEC): $(OBJ)
	$(CXX) -o $@ $(OBJ) $(LBLIBS)

clean:
	rm -rf $(OBJ) $(EXEC)