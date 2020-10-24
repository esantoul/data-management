CXX      := -c++
CXXFLAGS := -std=c++17 -pedantic-errors -Wall -Wextra -Werror
LDFLAGS  := -L/usr/lib -lstdc++ -lm
BUILD    := ./build
OBJ_DIR  := $(BUILD)/objects
APP_DIR  := $(BUILD)/apps
TARGET   := program
INCLUDE  := -Iinclude/
# SRC      :=                      \
#    $(wildcard src/module1/*.cpp) \
#    $(wildcard src/module2/*.cpp) \
#    $(wildcard src/module3/*.cpp) \
#    $(wildcard src/*.cpp)         \

# OBJECTS  := $(SRC:%.cpp=$(OBJ_DIR)/%.o)

# all: build $(APP_DIR)/$(TARGET)

# $(OBJ_DIR)/%.o: %.cpp
#    @mkdir -p $(@D)
#    $(CXX) $(CXXFLAGS) $(INCLUDE) -c $< -o $@ $(LDFLAGS)

# $(APP_DIR)/$(TARGET): $(OBJECTS)
#    @mkdir -p $(@D)
#    $(CXX) $(CXXFLAGS) -o $(APP_DIR)/$(TARGET) $^ $(LDFLAGS)

example:
	@mkdir -p $(APP_DIR)/examples
	$(CXX) $(CXXFLAGS) -o $(APP_DIR)/examples/$(EX).out $(INCLUDE) $(LDFLAGS) examples/$(EX)_example.cpp

example-snapshot: EX := snapshot
example-snapshot: example

example-static_mgr: EX := static_mgr
example-static_mgr: example

example-data_mgr: EX := data_mgr
example-data_mgr: example

.PHONY:build clean example example-snapshot example-static_mgr\
	# all debug release

build:
	@mkdir -p $(APP_DIR)
	@mkdir -p $(OBJ_DIR)

# debug: CXXFLAGS += -DDEBUG -g
# debug: all

# release: CXXFLAGS += -O2
# release: all

clean:
	-@rm -rvf $(BUILD)