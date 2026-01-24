CXX       := g++
NAME      := webserve
ARGS      ?=
CXXFLAGS  := -Wall -Wextra -Werror -std=c++98

ifdef release
    BUILD_TYPE := release
    CXXFLAGS   += -O3 -DNDEBUG
else
    BUILD_TYPE := debug
    CXXFLAGS   += -g3 -O0 -DDEBUG
endif

SRC_DIR    := src
OBJ_ROOT   := obj
OBJ_DIR    := $(OBJ_ROOT)/$(BUILD_TYPE)
BUILD_ROOT := build
BUILD_DIR  := $(BUILD_ROOT)/$(BUILD_TYPE)

# Pre-processor
CPPFLAGS := -I$(SRC_DIR) -MMD -MP
# Linker flags. Currently empty, but ready for use
LDFLAGS  :=

SRCS := $(shell find $(SRC_DIR) -name '*.cpp')
OBJS := $(SRCS:$(SRC_DIR)/%.cpp=$(OBJ_DIR)/%.o)
DEPS := $(OBJS:.o=.d)

.PHONY: all clean fclean re run release

all: $(NAME)

$(NAME): $(BUILD_DIR)/$(NAME)

# Linking
$(BUILD_DIR)/$(NAME): $(OBJS)
	@mkdir -p $(dir $@)
	$(CXX) $(OBJS) -o $@ $(LDFLAGS)

# Compiling
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@

release:
	@$(MAKE) all release=1

run: all
	./$(BUILD_DIR)/$(NAME) $(ARGS)

clean:
	rm -rf $(OBJ_ROOT)

fclean: clean
	rm -rf $(BUILD_ROOT)

re: fclean all

-include $(DEPS)
