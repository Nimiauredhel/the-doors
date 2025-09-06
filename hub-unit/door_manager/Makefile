SOURCE=*.c
PROGRAM=door_manager
EXE_NAME=$(PROGRAM)
ARGS=
BUILD_DIR=../build
EXE_PATH=$(BUILD_DIR)/$(EXE_NAME)
COMMON_LIB_PATH=/usr/lib/libhub_common.so
COMMON_LIB_SRC=../hub_common/*
INC=-I../../common -I../hub_common
LIBS=-lhub_common -lpthread -li2c -lrt
DEFAULT_FLAGS=-std=c99 -D_GNU_SOURCE
STRICT_FLAGS=$(DEFAULT_FLAGS) -Wall -pedantic -Wextra
STRICT_PREFIX=bear --
DEBUG_FLAGS=$(STRICT_FLAGS) -g -o0
FLAGS=
PREFIX=

.PHONY: default
default:
	$(MAKE) $(EXE_PATH) FLAGS="$(DEFAULT_FLAGS)"

.PHONY: strict
strict:                              
	$(MAKE) $(EXE_PATH) PREFIX="$(STRICT_PREFIX)" FLAGS="$(STRICT_FLAGS)"
                    
.PHONY: debug
debug:              
	$(MAKE) $(EXE_PATH) FLAGS="$(DEBUG_FLAGS)"

.PHONY: post-build
post-build:
	cp i2c2_init.sh $(BUILD_DIR)/

$(EXE_PATH): $(SOURCE) $(COMMON_LIB_PATH)
	mkdir -p $(BUILD_DIR)
	$(PREFIX) gcc $(SOURCE) $(FLAGS) $(INC) $(LIBS) -o $(EXE_PATH)
	$(MAKE) post-build

.PHONY: andrun
andrun:
	$(MAKE) default
	$(MAKE) run

.PHONY: clean
clean:
	rm $(EXE_PATH)
	rm compile_commands.json

.ONESHELL:
$(COMMON_LIB_PATH): $(COMMON_LIB_SRC)
	cd ../hub_common
	$(MAKE)

.PHONY: run
run:
	 cd $(BUILD_DIR); ./$(EXE_NAME) $(ARGS)

.PHONY: gdb
gdb:
	cd $(BUILD_DIR); gdb ./$(EXE_NAME) $(ARGS)

.PHONY: valgrind
valgrind:
	cd $(BUILD_DIR); valgrind -s --leak-check=yes --leak-check=full --show-leak-kinds=all --track-origins=yes ./$(EXE_NAME) $(ARGS)
