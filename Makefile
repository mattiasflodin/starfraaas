all: gl_driver
SRCS = quad_strategy.cpp gl_driver.cpp wavefront_object.cpp sorted_scene.cpp \
	cg_program_file.cpp gl_device.cpp device.cpp
LIBS = GL GLU glut Cg CgGL pthread

#CXX = g++4.0.2
CXXFLAGS = -g -Wall -W -march=pentium3 -D_GLIBCXX_DEBUG
#CXXFLAGS = -g -Wall -W -march=pentium4
LDFLAGS = -g
#CXXFLAGS = -Wall -W -O3 -fomit-frame-pointer -march=pentium4 -DNDEBUG -DUSE_SSE
#LDFLAGS = 

DEPENDENCIES = $(SRCS:.cpp=.d)
-include $(DEPENDENCIES)

OBJECTS = $(SRCS:.cpp=.o)
LDLIBS = $(addprefix -l,$(LIBS))

%.o : %.cpp
	$(COMPILE.cc) -MMD -MP $<

gl_driver: $(OBJECTS)
	$(LINK.o) $^ $(LDLIBS) -o $@

.PHONY: clean
clean:
	rm $(OBJECTS) $(DEPENDENCIES) gl_driver
