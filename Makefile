CXX ?= g++
SFML_PREFIX ?= /usr/local/opt/sfml@2
CXXFLAGS = -std=c++17 -Wall -Wextra -Wpedantic -O2 -MMD -MP -I$(SFML_PREFIX)/include -Iinclude
LDFLAGS = -L$(SFML_PREFIX)/lib -lsfml-graphics -lsfml-window -lsfml-system
TARGET = ant_colony
SOURCES = $(wildcard src/*.cpp)
OBJECTS = $(SOURCES:.cpp=.o)
DEPS = $(OBJECTS:.o=.d)
.PHONY: all clean debug asan
all: $(TARGET)
$(TARGET): $(OBJECTS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)
src/%.o: src/%.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@
clean:
	rm -f $(TARGET) $(OBJECTS) $(DEPS) simulation_stats.csv
debug:
	$(MAKE) CXXFLAGS="$(CXXFLAGS) -g -O0"
asan:
	$(MAKE) CXXFLAGS="$(CXXFLAGS) -g -O0 -fsanitize=address,undefined" LDFLAGS="$(LDFLAGS) -fsanitize=address,undefined"
-include $(DEPS)
