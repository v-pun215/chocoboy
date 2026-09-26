TARGET = chocoboy
SRC_DIR = src
OUT_DIR = output

CXX_STD = -std=c++20
DEBUG_FLAGS = -g

EMXX = em++
WASM_OUT = web

INCLUDES = -Iinclude/imgui -Iimgui

ifeq ($(OS),Windows_NT)
    # Windows native configuration
    CXX = g++
    # Point directly to your MSYS2 UCRT64 include and library folders
    CXXFLAGS = $(CXX_STD) $(DEBUG_FLAGS) $(INCLUDES) -I/msys64/ucrt64/include/SDL2 -IC:/msys64/ucrt64/include/SDL2
    LDFLAGS = -mconsole -L/msys64/ucrt64/lib -LC:/msys64/ucrt64/lib -lmingw32 -lSDL2main -lSDL2
    TARGET = chocoboy.exe
    
    # Use native Windows shell commands to prevent 'mkdir -p' or 'rm' crashes
    MKDIR = if not exist $(subst /,\,$(1)) mkdir $(subst /,\,$(1))
    RM = if exist $(subst /,\,$(1)) rmdir /s /q $(subst /,\,$(1))
else
    # Unix configuration (Linux / macOS)
    UNAME_S := $(shell uname -s)
    MKDIR = mkdir -p $(1)
    RM = rm -rf $(1)
    
    ifeq ($(UNAME_S),Darwin)
        # macOS
        CXX = clang++
        CXXFLAGS = $(CXX_STD) $(DEBUG_FLAGS) $(INCLUDES) -I/opt/homebrew/include/SDL2
        LDFLAGS = -L/opt/homebrew/lib -lSDL2
        TARGET = chocoboy
    else ifeq ($(UNAME_S),Linux)
        # Linux
        CXX = g++
        CXXFLAGS = $(CXX_STD) $(DEBUG_FLAGS) $(INCLUDES) -I/usr/include/SDL2
        LDFLAGS = -lSDL2
        TARGET = chocoboy
    endif
endif

# Separate source tracking
SRC_FILES = $(wildcard $(SRC_DIR)/*.cpp)
IMGUI_FILES = $(wildcard imgui/*.cpp)

# Map to object files in the output directory
OBJS = $(patsubst $(SRC_DIR)/%.cpp, $(OUT_DIR)/%.o, $(SRC_FILES)) \
       $(patsubst imgui/%.cpp, $(OUT_DIR)/imgui_%.o, $(IMGUI_FILES))

all: $(OUT_DIR)/$(TARGET)

$(OUT_DIR)/$(TARGET): $(OBJS)
	$(call MKDIR, $(OUT_DIR))
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

$(OUT_DIR)/%.o: $(SRC_DIR)/%.cpp
	$(call MKDIR, $(OUT_DIR))
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(OUT_DIR)/imgui_%.o: imgui/%.cpp
	$(call MKDIR, $(OUT_DIR))
	$(CXX) $(CXXFLAGS) -c $< -o $@

run: all
	cd $(OUT_DIR) && ./$(TARGET)

clean:
	$(call RM, $(OUT_DIR))

.PHONY: all run clean

wasm:
	$(call MKDIR, $(WASM_OUT))
	$(EMXX) \
		$(SRC_FILES) \
		$(IMGUI_FILES) \
		-Iinclude/imgui \
		-Iimgui \
		-sUSE_SDL=2 \
		-sSTACK_SIZE=1048576 \
		-sALLOW_MEMORY_GROWTH=0 \
		-sINITIAL_MEMORY=67108864 \
		-gsource-map \
		-sSAFE_HEAP=1 \
		-sSTACK_OVERFLOW_CHECK=2 \
		--preload-file roms \
		--preload-file test \
		-o $(WASM_OUT)/app.html
