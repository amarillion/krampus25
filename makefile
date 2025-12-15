#by Martijn van Iersel (amarillion@yahoo.com)

# BUILD=DEBUG
# BUILD=RELEASE

# TARGET=EMSCRIPTEN
# TARGET=LINUX

NAME=krampus25

LIBS = 
CFLAGS = -Iinclude -W -Wall

ALLEGRO_MODULES=allegro allegro_primitives allegro_font allegro_main allegro_dialog allegro_image allegro_audio allegro_acodec allegro_ttf allegro_color

ifeq ($(BUILD),RELEASE)
	CFLAGS += -O3 -s
	ALLEGRO_LIBS = $(addsuffix -5, $(ALLEGRO_MODULES))
else
ifeq ($(BUILD),DEBUG)
	CFLAGS += -g -DDEBUG
	ALLEGRO_LIBS = $(addsuffix -debug-5, $(ALLEGRO_MODULES))
else
$(error Unknown BUILD '$(BUILD)')
endif
endif

ifeq ($(TARGET),EMSCRIPTEN)
	CXX = em++
	LD = em++
	BINSUF = .html
	LIBS += `emconfigure pkg-config --libs allegro_monolith-static-5 sdl2`
	LIBS += -s USE_FREETYPE=1 -s USE_VORBIS=1 -s USE_OGG=1 -s USE_LIBJPEG=1 -s USE_LIBPNG=1 -s FULL_ES2=1 -s ASYNCIFY -s TOTAL_MEMORY=2147418112 -O3
	LIBS += --preload-file data@/data
else
ifeq ($(TARGET),LINUX)
	CXX = g++
	LD = g++
	BINSUF =
	LIBS += `pkg-config --libs $(ALLEGRO_LIBS)`
else
$(error Unknown TARGET '$(TARGET)')
endif
endif

BUILDDIR=build/$(BUILD)_$(TARGET)
OBJDIR=$(BUILDDIR)/obj

LFLAGS = 

BIN = $(BUILDDIR)/$(NAME)$(BINSUF)

$(shell mkdir -p $(OBJDIR) >/dev/null)

vpath %.cpp src

SRC = $(wildcard src/*.cpp)
OBJ = $(patsubst %.cpp, $(OBJDIR)/%.o, $(notdir $(SRC)))
DEP = $(patsubst %.cpp, $(OBJDIR)/%.d, $(notdir $(SRC)))

$(BIN) : $(OBJ) $(LIB)
	$(LD) $^ -o $@ $(LIBS) $(LFLAGS)

$(OBJ) : $(OBJDIR)/%.o : %.cpp
	$(CXX) $(CFLAGS) -MMD -c $< -o $@

$(OBJDIR):
	$(shell mkdir -p $(OBJDIR) >/dev/null)

.PHONY: clean
clean:
	rm -f $(OBJ) $(BIN)
