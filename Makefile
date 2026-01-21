CC = g++
FLAGS = -Wall -Wextra -Werror -std=c++98
BUILD_DIR = bin
SRC_DIR = src
OBJ_DIR = obj
SRCS = $(wildcard $(SRC_DIR)/*.cpp)
OBJS = $(SRCS:$(SRC_DIR)/%.cpp=$(OBJ_DIR)/%.o)
TARGET = webserve

all: $(BUILD_DIR)/$(TARGET)

$(BUILD_DIR)/$(TARGET): $(OBJS) | $(BUILD_DIR)
	$(CC) $(FLAGS) -o $@ $(OBJS)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp | $(OBJ_DIR)
	$(CC) $(FLAGS) -c $< -o $@

run: all
	./$(BUILD_DIR)/$(TARGET)

clean:
	rm -f $(OBJS)


fclean: clean
	rm -f $(BUILD_DIR)/$(TARGET)

re: clean