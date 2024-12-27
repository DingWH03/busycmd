# 定义编译器
CC = gcc

# 定义编译选项
CFLAGS = -Wall -Wextra -O2

INCLUDE_PATH = ./include

# 定义包含路径
INCLUDE = -I$(INCLUDE_PATH) -I$(INCLUDE_PATH)/utils

# 定义源文件路径
SRC_DIR = ./src

# 定义构建目录
BUILD_DIR = ./build

# 定义目标文件
TARGET = $(BUILD_DIR)/bin/busycmd

# 定义源文件
SRCS = $(wildcard $(SRC_DIR)/*.c) $(wildcard $(SRC_DIR)/**/*.c)

# 定义目标文件
OBJS = $(patsubst $(SRC_DIR)/%.c, $(BUILD_DIR)/%.o, $(SRCS))

# 默认目标
all: $(TARGET)

# 链接目标文件
$(TARGET): $(OBJS) | $(BUILD_DIR)/bin
	$(CC) $(CFLAGS) $(INCLUDE) -o $@ $^

# 创建 bin 目录
$(BUILD_DIR)/bin:
	mkdir -p $(BUILD_DIR)/bin

# 编译源文件
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c | $(BUILD_DIR)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(INCLUDE) -c $< -o $@

# 创建构建目录
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# 清理生成文件
clean:
	rm -f $(TARGET) $(OBJS)
	rm -rf $(BUILD_DIR)

# 运行目标文件
run: $(TARGET)
	$(TARGET)

.PHONY: all clean run