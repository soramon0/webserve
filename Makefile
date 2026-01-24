CXX      := g++
TARGET   := webserve

BUILD_DIR := bin
SRC_DIR   := src
OBJ_DIR   := obj

# Pre-processor
CPPFLAGS := -I$(SRC_DIR) -MMD -MP
# Compiler
CXXFLAGS := -Wall -Wextra -Werror -std=c++98
# Linker flags. Currently empty, but ready for use
LDFLAGS  :=

SRCS := $(shell find $(SRC_DIR) -name '*.cpp')
OBJS := $(SRCS:$(SRC_DIR)/%.cpp=$(OBJ_DIR)/%.o)
DEPS := $(OBJS:.o=.d)

.PHONY: all clean fclean re run

all: $(BUILD_DIR)/$(TARGET)

# Linking
$(BUILD_DIR)/$(TARGET): $(OBJS) | $(BUILD_DIR)
	$(CXX) $(OBJS) -o $@ $(LDFLAGS)

# Compiling
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@

$(BUILD_DIR) $(OBJ_DIR):
	mkdir -p $@

run: all
	./$(BUILD_DIR)/$(TARGET)

clean:
	rm -f $(OBJS) $(DEPS)

fclean:
	rm -rf $(BUILD_DIR) $(OBJ_DIR)

re: fclean all

-include $(DEPS)
