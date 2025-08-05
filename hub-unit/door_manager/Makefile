SOURCE= ../../common/*.c ../hub_common/*.c *.c
PROGRAM=door_manager
EXE_NAME=$(PROGRAM)
ARGS=
BUILD_DIR=../build/
EXE_PATH=$(BUILD_DIR)$(EXE_NAME)
INC= -I../../common -I../hub_common
LIBS= -lpthread -li2c
DEFAULT_FLAGS= -std=c99
STRICT_FLAGS= $(DEFAULT_FLAGS) -Wall -pedantic -Wextra
DEBUG_FLAGS= $(STRICT_FLAGS) -g -o0

default:
	mkdir -p $(BUILD_DIR)
	gcc $(SOURCE) $(DEFAULT_FLAGS) $(INC) $(LIBS) -o $(EXE_PATH)
	make post-build
                                     
strict:                              
	mkdir -p $(BUILD_DIR)
	bear -- gcc $(SOURCE) $(STRICT_FLAGS) $(INC) $(LIBS) -o $(EXE_PATH)
	make post-build
                    
debug:              
	mkdir -p $(BUILD_DIR)
	gcc $(SOURCE) $(DEBUG_FLAGS) $(INC) $(LIBS) -o $(EXE_PATH)
	make post-build

.ONESHELL:
run:
	 cd $(BUILD_DIR); ./$(EXE_NAME) $(ARGS)

andrun:
	make default
	make run

gdb:
	cd $(BUILD_DIR); gdb ./$(EXE_NAME) $(ARGS)

valgrind:
	cd $(BUILD_DIR); valgrind -s --leak-check=yes --leak-check=full --show-leak-kinds=all --track-origins=yes ./$(EXE_NAME) $(ARGS)

clean:
	rm -rf $(BUILD_DIR)
	rm -f compile_commands.json

post-build:
	cp i2c2_init.sh $(BUILD_DIR)

