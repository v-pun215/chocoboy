UNAME_S := $(shell uname -s)
TARGET = app
SRC_DIR = src
OUT_DIR = output

CXX_STD = -std=c++17
DEBUG_FLAGS = -g

# Fixed: Use relative paths for project-local includes
INCLUDES = -Iinclude/imgui -Iimgui

ifeq ($(UNAME_S),Darwin)
    CXX = /opt/homebrew/bin/g++-15
    CXXFLAGS = $(CXX_STD) $(DEBUG_FLAGS) $(INCLUDES) -I/opt/homebrew/include/SDL2
    LDFLAGS = -L/opt/homebrew/lib -lSDL2
else ifeq ($(UNAME_S),Linux)
    CXX = /usr/bin/g++
    CXXFLAGS = $(CXX_STD) $(DEBUG_FLAGS) $(INCLUDES) -I/usr/include/SDL2
    LDFLAGS = -lSDL2
else
    CXX = g++
    CXXFLAGS = $(CXX_STD) $(DEBUG_FLAGS) $(INCLUDES)
    LDFLAGS = 
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

# Build rule for your main source files
$(OUT_DIR)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(OUT_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Build rule for ImGui source files
$(OUT_DIR)/imgui_%.o: imgui/%.cpp
	@mkdir -p $(OUT_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

run: all
	cd $(OUT_DIR) && ./$(TARGET)

clean:
	rm -rf $(OUT_DIR)
.PHONY: all run clean