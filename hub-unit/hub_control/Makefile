SOURCE= ../../common/*.c ../hub_common/*.c *.c
PROGRAM=hub_control
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
	mkdir $(BUILD_DIR)
	gcc $(SOURCE) $(DEFAULT_FLAGS) $(INC) $(LIBS) -o $(EXE_PATH)
	make post-build
	make setcap
                                     
strict:                              
	mkdir $(BUILD_DIR)
	bear -- gcc $(SOURCE) $(STRICT_FLAGS) $(INC) $(LIBS) -o $(EXE_PATH)
	make post-build
	make setcap
                    
debug:              
	mkdir $(BUILD_DIR)
	gcc $(SOURCE) $(DEBUG_FLAGS) $(INC) $(LIBS) -o $(EXE_PATH)
	make post-build

.ONESHELL:
run:
	 cd $(BUILD_DIR); ./$(EXE_NAME) $(ARGS)

.ONESHELL:
run-tui:
	cd $(BUILD_DIR); bash start.sh

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

setcap:
	sudo setcap 'CAP_NET_BIND_SERVICE=ep' $(EXE_PATH)

post-build:
	#cp bash_scripts/* $(BUILD_DIR)
	#echo exe_name=$(EXE_NAME) > $(BUILD_DIR)exe_name.sh

