CC = g++
BUILD_DIR = bin
SRC_DIR = src
OBJ_DIR = obj
FLAGS = -Wall -Wextra -Werror -std=c++98 -I$(SRC_DIR) -MMD -MP
SRCS = $(wildcard $(SRC_DIR)/*.cpp)
OBJS = $(SRCS:$(SRC_DIR)/%.cpp=$(OBJ_DIR)/%.o)
DEPS = $(OBJS:.o=.d)
TARGET = webserve

all: $(BUILD_DIR)/$(TARGET)

$(BUILD_DIR)/$(TARGET): $(OBJS) | $(BUILD_DIR)
	$(CC) $(FLAGS) -o $@ $(OBJS)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp | $(OBJ_DIR)
	$(CC) $(FLAGS) -c $< -o $@ -MT $@

$(BUILD_DIR) $(OBJ_DIR):
	mkdir -p $@

run: all
	./$(BUILD_DIR)/$(TARGET)

clean:
	rm -f $(OBJS) $(DEPS)

fclean:
	rm -rf $(BUILD_DIR) $(OBJ_DIR)

re: fclean all

.PHONY: all clean fclean re run

-include $(DEPS)