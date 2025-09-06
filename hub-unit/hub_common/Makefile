SOURCE=../../common/*.c *.c
PROGRAM=libhub_common.so
EXE_NAME=$(PROGRAM)
ARGS=
BUILD_DIR=/usr/lib
EXE_PATH=$(BUILD_DIR)/$(EXE_NAME)
INC=-I../../common
LIBS=-lpthread -lrt
DEFAULT_FLAGS=-shared -fPIC -std=c99 -D_GNU_SOURCE
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

$(EXE_PATH): $(SOURCE)
	mkdir -p $(BUILD_DIR)
	sudo $(PREFIX) gcc $(SOURCE) $(FLAGS) $(INC) $(LIBS) -o $(EXE_PATH)
	sudo ldconfig

.PHONY: clean
clean:
	sudo rm $(EXE_PATH)
	rm compile_commands.json
