CC = gcc
BUILD_DIR = ./build
MAIN_DIR = ./src/main
TEST_DIR = ./src/test

build:
	@mkdir $(BUILD_DIR)

clean:
	@rm -rf $(BUILD_DIR)

MAIN_OUT = $(BUILD_DIR)/main

main: build
	@$(CC) -o $(MAIN_OUT) $(MAIN_DIR)/*.c
	@$(MAIN_OUT)

TEST_OUT = $(BUILD_DIR)/test
TEST_FILES_WITH_MAIN = $(wildcard $(MAIN_DIR)/*.c $(TEST_DIR)/*.c)
TEST_FILES = $(filter-out $(MAIN_DIR)/main.c, $(TEST_FILES_WITH_MAIN))

TEST_FLAGS = BUILD_GRAMMAR_TEST

.PHONY: test

test: build
	@$(CC) -o $(TEST_OUT) $(TEST_FILES) -I$(MAIN_DIR) -D$(TEST_FLAGS)
	@$(TEST_OUT)