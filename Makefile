CC=g++
LNFLAGS=-lSDL2
SRCDIR=./nes
SRCDIRS=$(shell find $(SRCDIR) -type d)
SRC=$(foreach dir, $(SRCDIRS), $(wildcard $(dir)/*.cpp))

ifndef DEBUG
	CXXFLAGS=-std=c++11 -MMD -MP -O3
	OBJDIR=Release
	EXECUTABLE=Release/nes
else
	CXXFLAGS=-std=c++11 -g -MMD -MP -D _DEBUG
	OBJDIR=Debug
	EXECUTABLE=Debug/nes
endif

OBJDIRS=$(pathsubst $(SRCDIRS)/%,$(OBJDIRS)/%,$(SRCDIRS))
_OBJ=$(SRC:.cpp=.o)
OBJ=$(patsubst $(SRCDIR)/%,$(OBJDIR)/%,$(_OBJ))
DEPS = ${OBJ:.o=.d}

.PHONY: clean

strela: $(EXECUTABLE)

$(EXECUTABLE): $(OBJ)
	$(CC) $^ $(CXXFLAGS) $(LNFLAGS) -o $@

$(OBJDIR)/%.o: $(SRCDIR)/%.cpp
	@[ -d $(@D) ] || mkdir -p $(@D)
	$(CC) $< $(CXXFLAGS) -c -o $@

clean:
	rm -rf Release Debug

-include ${DEPS}
