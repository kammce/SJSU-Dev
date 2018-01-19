# Allow settiing a project name from the environment, default to firmware.
# Only affects the name of the generated binary.
# TODO: Set this from the directory this makefile is stored in
PROJ 			?= firmware
# Affects what DBC is generated for SJSUOne board
ENTITY 			?= DBG

# IMPORTANT: Must be accessible via the PATH variable!!!
CC              = arm-none-eabi-gcc
CPPC            = arm-none-eabi-g++
OBJDUMP         = arm-none-eabi-objdump
SIZEC           = arm-none-eabi-size
OBJCOPY         = arm-none-eabi-objcopy
NM 		        = arm-none-eabi-nm

# Internal build directories
OBJ_DIR			= obj
BIN_DIR			= bin
DBC_DIR			= _can_dbc

define n


endef

ifndef SJSUONEDEV
$(error $n$n=============================================$nSJSUOne environment variables not set.$nPLEASE run "source env.sh"$n=============================================$n$n)
endif

CFLAGS = -mcpu=cortex-m3 \
	-D DISABLE_WATCHDOG\
    -mthumb -g -Os -fmessage-length=0 \
    -ffunction-sections -fdata-sections \
    -Wall -Wshadow -Wlogical-op \
    -Wfloat-equal -DBUILD_CFG_MPU=0 \
    -fabi-version=0 \
    -fno-exceptions \
    -I"$(LIB_DIR)/" \
    -I"$(LIB_DIR)/newlib" \
    -I"$(LIB_DIR)/L0_LowLevel" \
    -I"$(LIB_DIR)/L1_FreeRTOS" \
    -I"$(LIB_DIR)/L1_FreeRTOS/trace" \
    -I"$(LIB_DIR)/L1_FreeRTOS/include" \
    -I"$(LIB_DIR)/L1_FreeRTOS/portable" \
    -I"$(LIB_DIR)/L1_FreeRTOS/portable/no_mpu" \
    -I"$(LIB_DIR)/L2_Drivers" \
    -I"$(LIB_DIR)/L2_Drivers/base" \
    -I"$(LIB_DIR)/L3_Utils" \
    -I"$(LIB_DIR)/L3_Utils/tlm" \
    -I"$(LIB_DIR)/L4_IO" \
    -I"$(LIB_DIR)/L4_IO/fat" \
    -I"$(LIB_DIR)/L4_IO/wireless" \
    -I"$(LIB_DIR)/L5_Application" \
    -I"$(LIB_DIR)/L5_Assembly" \
    -I"L2_Drivers" \
    -I"L3_Utils" \
    -I"L4_IO" \
    -I"L5_Application" \
    -I"L5_Assembly" \
    -I"$(DBC_DIR)" \
    -MMD -MP -c

LINKFLAGS = -mcpu=cortex-m3 \
	-Os -mthumb \
	-fmessage-length=0 -ffunction-sections -fdata-sections \
	-Wall -Wshadow -Wlogical-op -Wfloat-equal \
	-T $(LIB_DIR)/loader.ld \
	-nostartfiles \
	-Xlinker \
	--gc-sections -Wl,-Map,"$(MAP)" \
	-specs=nano.specs

DBC_BUILD        	= $(DBC_DIR)/generated_can.h
LIBRARIES			= $(shell find "$(LIB_DIR)" -name '*.c' -o -name '*.cpp')
SOURCES				= $(shell find L5_Application L5_Assembly \
 						 -name '*.c' -o\
						 -name '*.s' -o \
						 -name '*.S' -o \
						 -name '*.cpp' \
						 -not -path './test/*')
COMPILABLES 		= $(LIBRARIES) $(SOURCES)

# $(patsubst %.cpp,%.o, LIST) 		: Replace .cpp -> .o
# $(patsubst %.c,%.o, LIST)			: Replace .c -> .o
# $(patsubst src/%,%, LIST) 		: Replace src/path/file.o -> path/file.o
# $(addprefix $(OBJ_DIR)/, LIST) 	: Add OBJ DIR to path (path/file.o -> obj/path/file.o)
OBJECT_FILES 		= $(addprefix $(OBJ_DIR)/, \
						$(patsubst %.S,%.o, \
							$(patsubst %.s,%.o, \
								$(patsubst %.c,%.o, \
									$(patsubst %.cpp,%.o, \
										$(COMPILABLES) \
									) \
								) \
							) \
						) \
					)
EXECUTABLE			= $(BIN_DIR)/$(PROJ).elf
SYMBOL_TABLE 		= $(BIN_DIR)/symbol-table.c
HEX					= $(EXECUTABLE:.elf=.hex)
SYMBOLS_HEX			= $(EXECUTABLE:.elf=.symbols.hex)
LIST				= $(EXECUTABLE:.elf=.lst)
SYMBOLS_LIST		= $(EXECUTABLE:.elf=.symbols.lst)
SIZE				= $(EXECUTABLE:.elf=.siz)
SYMBOLS_SIZE		= $(EXECUTABLE:.elf=.symbols.siz)
MAP					= $(EXECUTABLE:.elf=.map)
SYMBOLS_MAP			= $(EXECUTABLE:.elf=.symbols.map)
SYMBOLS				= $(EXECUTABLE:.elf=.sym)
SYMBOLS_EXECUTABLE	= $(EXECUTABLE:.elf=.symbols.elf)
SYMBOLS_OBJECT 		= $(SYMBOLS).o

.PHONY: build clean cleaninstall flash telemetry monitor show-obj-list

default:
	@echo "List of available targets:"
	@echo "    build         - builds firmware project"
	@echo "    nosym-build   - builds firmware project without embeddeding symbol table"
	@echo "    flash         - builds and installs firmware on to SJOne board"
	@echo "    nosym-flash   - builds and installs firmware on to SJOne board without embeddeding symbol table"
	@echo "    telemetry     - will launch telemetry interface"
	@echo "    clean         - cleans project folder"
	@echo "    cleaninstall  - cleans, builds and installs firmware"
	@echo "    show-obj-list - Shows all object files that will be compiled"

nosym-build: $(DBC_DIR) $(OBJ_DIR) $(BIN_DIR) $(SIZE) $(LIST) $(HEX)

build: $(DBC_DIR) $(OBJ_DIR) $(BIN_DIR) $(SYMBOLS_SIZE) $(SYMBOLS_LIST) $(SYMBOLS_HEX)

cleaninstall: clean build flash

show-obj-list:
	@echo $(OBJECT_FILES)

print-%  : ; @echo $* = $($*)

$(SYMBOLS_HEX): $(SYMBOLS_EXECUTABLE)
	@echo 'Invoking: Cross ARM GNU Create Symbol Linked Flash Image'
	@$(OBJCOPY) -O ihex "$<" "$@"
	@echo 'Finished building: $@'
	@echo ' '

$(HEX): $(EXECUTABLE)
	@echo 'Invoking: Cross ARM GNU Create Flash Image'
	@$(OBJCOPY) -O ihex "$<" "$@"
	@echo 'Finished building: $@'
	@echo ' '

$(SYMBOLS_SIZE): $(SYMBOLS_EXECUTABLE)
	@echo 'Invoking: Cross ARM GNU Print Size'
	@$(SIZEC) --format=berkeley "$<"
	@echo 'Finished building: $@'
	@echo ' '

$(SIZE): $(EXECUTABLE)
	@echo 'Invoking: Cross ARM GNU Print Size'
	@$(SIZEC) --format=berkeley "$<"
	@echo 'Finished building: $@'
	@echo ' '

$(SYMBOLS_LIST): $(SYMBOLS_EXECUTABLE)
	@echo 'Invoking: Cross ARM GNU Create Assembly Listing'
	@$(OBJDUMP) --source --all-headers --demangle --line-numbers --wide "$<" > "$@"
	@echo 'Finished building: $@'
	@echo ' '

$(LIST): $(EXECUTABLE)
	@echo 'Invoking: Cross ARM GNU Create Assembly Listing'
	@$(OBJDUMP) --source --all-headers --demangle --line-numbers --wide "$<" > "$@"
	@echo 'Finished building: $@'
	@echo ' '

$(SYMBOLS_EXECUTABLE): $(SYMBOLS_OBJECT)
	@echo 'Linking: FINAL Symbol Table Linked EXECUTABLE'
	@mkdir -p "$(dir $@)"
	@$(CPPC) $(LINKFLAGS) -o "$@" $(SYMBOLS_OBJECT) $(OBJECT_FILES)
	@echo 'Finished building target: $@'
	@echo ' '

$(SYMBOLS_OBJECT): $(SYMBOL_TABLE)
	@echo ' '
	@echo 'Invoking: Cross ARM GNU Generating Symbol Table Object File'
	@$(CC) $(CFLAGS) -std=gnu11 -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $@'
	@echo ' '

$(SYMBOL_TABLE): $(SYMBOLS)
	@echo ' '
	@echo 'Generating: Symbol Table C file'
	@# Copying firmware.sym to .c file
	@cat "$<" > "$@"
	@# Remove everything that is not a function (text/code) symbols
	@sed -i '/ T /!d' "$@"
	@sed -i '/ T __/d' "$@"
	@sed -i '/ T _/d' "$@"
	@sed -i '/ T operator /d' "$@"
	@sed -i '/ T typeinfo for/d' "$@"
	@sed -i '/ T typeinfo name for /d' "$@"
	@sed -i '/ T typeinfo name for /d' "$@"
	@sed -i '/ T vtable for /d' "$@"
	@sed -i '/ T vtable for /d' "$@"
	@# Prepend " to each line
	@sed -i 's/^/\t"/' "$@"
	@# Append " to each line
	@sed -i 's/$$/\\n\"/' "$@"
	@# Append variable declaration
	@sed -i '1s;^;__attribute__((section(".symbol_table"))) const char APP_SYM_TABLE[] =\n{\n;' "$@"
	@# append it with a curly brace and semicolon
	@echo "\n};" >> "$@"
	@echo ' '

$(SYMBOLS): $(EXECUTABLE)
	@echo 'Generating: Cross ARM GNU NM Generate Symbol Table'
	@$(NM) -C "$<" > "$@"
	@echo 'Finished building: $@'
	@echo ' '

$(EXECUTABLE): $(DBC_BUILD) $(OBJECT_FILES)
	@echo 'Invoking: Cross ARM C++ Linker'
	@mkdir -p "$(dir $@)"
	@$(CPPC) $(LINKFLAGS) -o "$@" $(OBJECT_FILES)
	@echo 'Finished building target: $@'

$(OBJ_DIR)/%.o: %.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: Cross ARM C++ Compiler'
	@mkdir -p "$(dir $@)"
	@$(CPPC) $(CFLAGS) -std=gnu++17 -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

$(OBJ_DIR)/%.o: %.c
	@echo 'Building file: $<'
	@echo 'Invoking: Cross ARM C Compiler'
	@mkdir -p "$(dir $@)"
	@$(CC) $(CFLAGS) -std=gnu11 -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

$(OBJ_DIR)/%.o: %.s
	@echo 'Building Assembly file: $<'
	@echo 'Invoking: Cross ARM C Compiler'
	@mkdir -p "$(dir $@)"
	@$(CC) $(CFLAGS) -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

$(OBJ_DIR)/%.o: %.S
	@echo 'Building Assembly file: $<'
	@echo 'Invoking: Cross ARM C Compiler'
	@mkdir -p "$(dir $@)"
	@$(CC) $(CFLAGS) -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

$(OBJ_DIR)/%.o: $(LIB_DIR)/%.cpp
	@echo 'Building C++ file: $<'
	@echo 'Invoking: Cross ARM C++ Compiler'
	@mkdir -p "$(dir $@)"
	@$(CPPC) $(CFLAGS) -std=gnu++17 -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

$(OBJ_DIR)/%.o: $(LIB_DIR)/%.c
	@echo 'Building C file: $<'
	@echo 'Invoking: Cross ARM C Compiler'
	@mkdir -p "$(dir $@)"
	@$(CC) $(CFLAGS) -std=gnu11 -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

$(DBC_BUILD):
	python "$(LIB_DIR)/$(DBC_DIR)/dbc_parse.py" -i "$(LIB_DIR)/$(DBC_DIR)/243.dbc" -s $(ENTITY) > $(DBC_BUILD)

$(DBC_DIR):
	mkdir -p $(DBC_DIR)

$(OBJ_DIR):
	@echo 'Creating Objects Folder: $<'
	mkdir $(OBJ_DIR)

$(BIN_DIR):
	@echo 'Creating Binary Folder: $<'
	mkdir $(BIN_DIR)

clean:
	rm -fR $(OBJ_DIR) $(BIN_DIR) $(DBC_DIR)

nosym-flash: build
	hyperload $(SJSUONEDEV) $(HEX)

flash: build
	hyperload $(SJSUONEDEV) $(SYMBOLS_HEX)

telemetry:
	@telemetry