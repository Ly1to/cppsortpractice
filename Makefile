CXX ?= g++
CPPFLAGS += -Iinclude
CXXFLAGS ?= -std=c++11 -Wall -Wextra -Wpedantic
LDFLAGS ?=
LDLIBS ?=

TARGET := build/OrdCsltion
SOURCE_DIR := src
OBJECT_DIR := build/obj
RIDES ?= rides_1k.txt
PYTHON ?= python3
GENERATE_COUNT ?= 10000
GENERATE_SEED ?= 42
GENERATED_RIDES ?= rides_generated.txt

SOURCES := $(wildcard $(SOURCE_DIR)/*.cpp)
OBJECTS := $(patsubst $(SOURCE_DIR)/%.cpp,$(OBJECT_DIR)/%.o,$(SOURCES))
DEPENDENCIES := $(OBJECTS:.o=.d)

.PHONY: all run generate clean rebuild

all: $(TARGET)

$(TARGET): $(OBJECTS)
	@mkdir -p $(dir $@)
	$(CXX) $(LDFLAGS) $^ $(LDLIBS) -o $@

$(OBJECT_DIR)/%.o: $(SOURCE_DIR)/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -MMD -MP -c $< -o $@

run: $(TARGET)
	./$(TARGET) $(RIDES)

generate:
	$(PYTHON) tools/generate_rides.py --count $(GENERATE_COUNT) \
		--seed $(GENERATE_SEED) --output $(GENERATED_RIDES)

clean:
	$(RM) -r build

rebuild: clean all

-include $(DEPENDENCIES)
