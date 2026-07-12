UNAME_S := $(shell uname -s)
TARGET = chocoboy
SRC_DIR = src
OUT_DIR = output

CXX_STD = -std=c++20
DEBUG_FLAGS = -g

EMXX = em++
WASM_OUT = web

INCLUDES = -Iinclude/imgui -Iimgui

ifeq ($(OS),Windows_NT)
    CXX = g++
    CXXFLAGS = $(CXX_STD) $(DEBUG_FLAGS) $(INCLUDES) -I/mingw64/include/SDL2
    LDFLAGS = -lmingw32 -lSDL2main -lSDL2
    TARGET = chocoboy.exe
else
    UNAME_S := $(shell uname -s)
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
# Prefixing imgui objects to prevent potential name collisions
OBJS = $(patsubst $(SRC_DIR)/%.cpp, $(OUT_DIR)/%.o, $(SRC_FILES)) \
       $(patsubst imgui/%.cpp, $(OUT_DIR)/imgui_%.o, $(IMGUI_FILES))

all: $(OUT_DIR)/$(TARGET)

$(OUT_DIR)/$(TARGET): $(OBJS)
	@mkdir -p $(OUT_DIR)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

$(OUT_DIR)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(OUT_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(OUT_DIR)/imgui_%.o: imgui/%.cpp
	@mkdir -p $(OUT_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

run: all
	cd $(OUT_DIR) && ./$(TARGET)

clean:
	rm -rf $(OUT_DIR)
.PHONY: all run clean

wasm:
	mkdir -p $(WASM_OUT)

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