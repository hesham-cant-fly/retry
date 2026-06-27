CC := clang

SRC_DIR := source
INC_DIR := include
BUILD_DIR := .build

SRCS := $(wildcard $(SRC_DIR)/*.c)
OBJS := $(patsubst $(SRC_DIR)/%.c, $(BUILD_DIR)/%.o, $(SRCS))
DEPS := $(OBJS:.o=.d)

TARGET := retry

.PHONY: all debug release clean compile_flags.txt

BUILD ?= debug

ifeq ($(BUILD),debug)
CFLAGS := -Wall -Wextra -pedantic -std=c11 -O0 -g
CFLAGS += -fsanitize=address,undefined -fno-omit-frame-pointer
LDFLAGS := -fsanitize=address,undefined
else
CFLAGS := -Wall -Wextra -pedantic -std=c11 -O2
LDFLAGS := -static
endif

CFLAGS += -I$(INC_DIR)

all: compile_flags.txt $(TARGET)

debug:
	$(MAKE) BUILD=debug

release:
	$(MAKE) BUILD=release

$(TARGET): $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $^

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -MMD -MP -c $< -o $@

$(BUILD_DIR):
	mkdir -p $@

compile_flags.txt:
	@printf '%s\n' $(filter-out -MMD -MP, $(CFLAGS)) > $@

-include $(DEPS)

clean:
	rm -rf $(BUILD_DIR) $(TARGET) compile_flags.txt
