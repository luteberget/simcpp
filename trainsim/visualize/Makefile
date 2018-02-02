#
# Cross Platform Makefile
# Compatible with MSYS2/MINGW, Ubuntu 14.04.1 and Mac OS X
#
# You will need SDL2 (http://www.libsdl.org):
# Linux:
#   apt-get install libsdl2-dev
# Mac OS X:
#   brew install sdl2
# MSYS2:
#   pacman -S mingw-w64-i686-SDL
#

#CXX = g++
#CXX = clang++

EXE = simvis
OBJS = main.o imgui_impl_sdl_gl3.o ../traindynamics.o plotparser.o trainplot.o ../parser.o
OBJS += imgui/imgui.o imgui/imgui_demo.o imgui/imgui_draw.o
OBJS += imgui/examples/libs/gl3w/GL/gl3w.o

UNAME_S := $(shell uname -s)


ifeq ($(UNAME_S), Linux) #LINUX
	ECHO_MESSAGE = "Linux"
	LIBS = -lGL -ldl `sdl2-config --libs`

	CXXFLAGS = -Iimgui -Iimgui/examples/libs/gl3w `sdl2-config --cflags`
	CXXFLAGS += -Wall -Wformat
	CFLAGS = $(CXXFLAGS)
endif

ifeq ($(UNAME_S), Darwin) #APPLE
	ECHO_MESSAGE = "Mac OS X"
	LIBS = -framework OpenGL -framework Cocoa -framework IOKit -framework CoreVideo `sdl2-config --libs`

	CXXFLAGS = -I../../ -I../libs/gl3w -I/usr/local/include `sdl2-config --cflags`
	CXXFLAGS += -Wall -Wformat
	CFLAGS = $(CXXFLAGS)
endif

ifeq ($(findstring MINGW,$(UNAME_S)),MINGW)
   ECHO_MESSAGE = "Windows"
   LIBS = -lgdi32 -lopengl32 -limm32 `pkg-config --static --libs sdl2`

   CXXFLAGS = -I../../ -I../libs/gl3w `pkg-config --cflags sdl2`
   CXXFLAGS += -Wall -Wformat
   CFLAGS = $(CXXFLAGS)
endif


.cpp.o:
	$(CXX) -std=c++11 $(CXXFLAGS) -c -o $@ $<

all: $(EXE)
	@echo Build complete for $(ECHO_MESSAGE)

$(EXE): $(OBJS)
	$(CXX) -std=c++11 -o $(EXE) $(OBJS) $(CXXFLAGS) $(LIBS)

clean:
	rm $(EXE) $(OBJS)
