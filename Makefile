CXX      := g++
NAME     := webserve
ARGS     ?=
CXXFLAGS := -Wall -Wextra -Werror -std=c++98

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

# Build Configuration
ifdef release
    BUILD_TYPE := release
    CXXFLAGS   += -O3 -DNDEBUG
else
    BUILD_TYPE := debug
    CXXFLAGS   += -g3 -O0 -DDEBUG
endif

ifdef sanitize
    CXXFLAGS   += -fsanitize=address -fno-omit-frame-pointer
    LDFLAGS    += -fsanitize=address
endif

# Directories
SRC_DIR    := src
OBJ_DIR    := obj/$(BUILD_TYPE)
BUILD_DIR  := build/$(BUILD_TYPE)

# Pre-processor
CPPFLAGS := -I$(SRC_DIR) -MMD -MP
LDFLAGS  +=

# Auto-generate full paths
SRCS := $(shell find $(SRC_DIR) -name '*.cpp')
OBJS := $(SRCS:$(SRC_DIR)/%.cpp=$(OBJ_DIR)/%.o)
DEPS := $(OBJS:.o=.d)

# Safety Net
.DELETE_ON_ERROR:

.PHONY: all clean fclean re run release help

all: $(NAME)

$(NAME): $(BUILD_DIR)/$(NAME)

# Linking
$(BUILD_DIR)/$(NAME): $(OBJS)
	@mkdir -p $(dir $@)
	@printf "$(BLUE)Linking $(NAME) ($(BUILD_TYPE))...$(RESET)\n"
	$(Q)$(CXX) $(OBJS) -o $@ $(LDFLAGS)
	@printf "$(GREEN)Build successful! Binary: $@$(RESET)\n"

# Compiling
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(dir $@)
	@printf "$(YELLOW)Compiling $<...$(RESET)\n"
	$(Q)$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@

release:
	@$(MAKE) all release=1

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
	@printf "  $(GREEN)make$(RESET)            : Build Debug version (default)\n"
	@printf "  $(GREEN)make release=1$(RESET)  : Build Release version\n"
	@printf "  $(GREEN)make sanitize=1$(RESET) : Build with ASan\n"
	@printf "  $(GREEN)make run$(RESET)        : Run the program\n"

-include $(DEPS)
