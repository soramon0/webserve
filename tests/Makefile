CXX      := g++
CXXFLAGS := -Wall -Wextra -Werror -std=c++98
LDFLAGS  :=
NAME     := webserv_test

# Colors
GREEN   := \033[1;32m
YELLOW  := \033[1;33m
BLUE    := \033[1;34m
MAGENTA := \033[1;35m
RESET   := \033[0m

# Verbose Mode
ifndef V
    Q := @
else
    Q :=
endif

BUILD_TYPE := test
CXXFLAGS   += -g3 -O0 -DDEBUG=1

# Directories
OBJ_DIR   := obj/$(BUILD_TYPE)
BUILD_DIR := build/$(BUILD_TYPE)

# Production sources (exclude program entry — tests supply their own main)
PROJ_SRCS := $(shell find ../src -name '*.cpp' ! -name 'main.cpp')
TEST_SRCS := $(shell find src -name '*.cpp')
SRCS      := $(PROJ_SRCS) $(TEST_SRCS)

PROJ_OBJS := $(patsubst ../src/%.cpp,$(OBJ_DIR)/proj/%.o,$(PROJ_SRCS))
TEST_OBJS := $(patsubst src/%.cpp,$(OBJ_DIR)/test/%.o,$(TEST_SRCS))
OBJS      := $(PROJ_OBJS) $(TEST_OBJS)
DEPS      := $(OBJS:.o=.d)

# Pre-processor
CPPFLAGS := -I../src -Isrc -I../include -MMD -MP

# Safety Net
.DELETE_ON_ERROR:

.PHONY: all clean fclean re run help

all: $(NAME)

$(NAME): $(BUILD_DIR)/$(NAME)

# Linking
$(BUILD_DIR)/$(NAME): $(OBJS)
	@mkdir -p $(dir $@)
	@printf "$(BLUE)Linking $(NAME) ($(BUILD_TYPE))...$(RESET)\n"
	$(Q)$(CXX) $(OBJS) -o $@ $(LDFLAGS)
	@printf "$(GREEN)Build successful! Binary: $@$(RESET)\n"

# Compiling production code
$(OBJ_DIR)/proj/%.o: ../src/%.cpp
	@mkdir -p $(dir $@)
	@printf "$(YELLOW)Compiling $<...$(RESET)\n"
	$(Q)$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@

# Compiling tests
$(OBJ_DIR)/test/%.o: src/%.cpp
	@mkdir -p $(dir $@)
	@printf "$(YELLOW)Compiling $<...$(RESET)\n"
	$(Q)$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@

run: $(NAME)
	@printf "$(MAGENTA)Running $(NAME)...$(RESET)\n"
	./$(BUILD_DIR)/$(NAME) $(ARGS)

clean:
	@printf "$(YELLOW)Cleaning objects...$(RESET)\n"
	$(Q)rm -rf $(firstword $(subst /, ,$(OBJ_DIR)))

fclean: clean
	@printf "$(YELLOW)Removing executables...$(RESET)\n"
	$(Q)rm -rf $(firstword $(subst /, ,$(BUILD_DIR)))

re: fclean all

help:
	@printf "$(MAGENTA)Available targets:$(RESET)\n"
	@printf "  $(GREEN)make$(RESET)            : Build test binary ($(NAME))\n"
	@printf "  $(GREEN)make run$(RESET)        : Run test binary\n"

-include $(DEPS)
