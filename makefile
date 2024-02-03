CXX = c++ 
CXXFLAGS = -std=c++17 -g3 #-Wall -Wextra -Werror
LDFLAGS = -fsanitize=address

SRC_DIR = ./

OBJ_DIR = ./obj

INC_DIR = -I./ -I./SDL2 -I./imgui -I./volk -I./vkbootstrap -I./tinyobjloader -I./vma -I./stb_image -I./particle/include -I./stableFluids/include -I./cloud/include

VK_LIB_DIR := $(shell echo $$DYLD_LIBRARY_PATH)

BREW_LIB_DIR = /Users/schoe/.brew/lib

TARGET = ./a.out

SOURCES = $(wildcard $(SRC_DIR)/*.cpp ./imgui/*.cpp ./volk/*.cpp ./vkbootstrap/*.cpp ./tinyobjloader/*.cpp ./particle/*.cpp ./stableFluids/*.cpp ./cloud/*.cpp) 

OBJECTS = $(patsubst $(SRC_DIR)/%.cpp,$(OBJ_DIR)/%.o,\
$(patsubst ./imgui/%.cpp,$(OBJ_DIR)/%.o,\
$(patsubst ./volk/%.cpp,$(OBJ_DIR)/%.o,\
$(patsubst ./vkbootstrap/%.cpp,$(OBJ_DIR)/%.o,\
$(patsubst ./tinyobjloader/%.cpp,$(OBJ_DIR)/%.o,\
$(patsubst ./particle/%.cpp,$(OBJ_DIR)/%.o,\
$(patsubst ./stableFluids/%.cpp,$(OBJ_DIR)/%.o,\
$(patsubst ./cloud/%.cpp,$(OBJ_DIR)/%.o,\
$(SOURCES)))))))))

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CXX) $(LDFLAGS) -o $@ $^ -L$(BREW_LIB_DIR) -lSDL2 -lGLEW -L$(VK_LIB_DIR) -rpath $(VK_LIB_DIR) -lvulkan -L$(VK_LIB_DIR) -rpath $(VK_LIB_DIR) -lvulkan.1.3.250 -framework Cocoa -framework IOKit

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	$(CXX) $(CXXFLAGS) $(INC_DIR) -c $< -o $@

$(OBJ_DIR)/%.o: ./imgui/%.cpp
	$(CXX) $(CXXFLAGS) $(INC_DIR) -c $< -o $@

$(OBJ_DIR)/%.o: ./volk/%.cpp
	$(CXX) $(CXXFLAGS) $(INC_DIR) -c $< -o $@

$(OBJ_DIR)/%.o: ./vkbootstrap/%.cpp
	$(CXX) $(CXXFLAGS) $(INC_DIR) -c $< -o $@

$(OBJ_DIR)/%.o: ./tinyobjloader/%.cpp
	$(CXX) $(CXXFLAGS) $(INC_DIR) -c $< -o $@

$(OBJ_DIR)/%.o: ./particle/%.cpp
	$(CXX) $(CXXFLAGS) $(INC_DIR) -c $< -o $@

$(OBJ_DIR)/%.o: ./stableFluids/%.cpp
	$(CXX) $(CXXFLAGS) $(INC_DIR) -c $< -o $@

$(OBJ_DIR)/%.o: ./cloud/%.cpp
	$(CXX) $(CXXFLAGS) $(INC_DIR) -c $< -o $@

clean:
	rm -f $(OBJ_DIR)/*.o

fclean: clean
	rm -f $(TARGET)

re : 
	${MAKE} fclean 
	${MAKE} all

.PHONY: all clean fclean re