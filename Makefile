UNAME_S := $(shell uname -s)
TARGET = app
SRC_DIR = src
OUT_DIR = output

CXX_STD = -std=c++17
DEBUG_FLAGS = -g

ifeq ($(UNAME_S),Darwin)
    CXX = /opt/homebrew/bin/g++-15
    CXXFLAGS = $(CXX_STD) $(DEBUG_FLAGS)
    LDFLAGS = 
else ifeq ($(UNAME_S),Linux)
    CXX = /usr/bin/g++
    CXXFLAGS = $(CXX_STD) $(DEBUG_FLAGS) -I/usr/include/SDL2
    LDFLAGS = -lSDL2
else
    CXX = g++
    CXXFLAGS = $(CXX_STD) $(DEBUG_FLAGS)
    LDFLAGS = 
endif
SRCS = $(wildcard $(SRC_DIR)/*.cpp)

OBJS = $(patsubst $(SRC_DIR)/%.cpp, $(OUT_DIR)/%.o, $(SRCS))

all: $(OUT_DIR)/$(TARGET)

$(OUT_DIR)/$(TARGET): $(OBJS)
	@mkdir -p $(OUT_DIR)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

$(OUT_DIR)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(OUT_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@
run: all
	cd $(OUT_DIR) && ./$(TARGET)

clean:
	rm -rf $(OUT_DIR)

.PHONY: all run clean