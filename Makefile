CXX=g++
RM=rm -f
CXXFLAGS=-g -march=native -std=c++23 -O3
LDFLAGS=-g 
LDLIBS=-lboost_program_options -lboost_filesystem -lboost_system

SRCS= \
	main.cpp \
	commands.cpp \
	cwordle.cpp \
	styled_text.cpp \
	wordle_word.cpp \
	word_list.cpp \
	dictionary.cpp \
	random.cpp \
	vocabulary.cpp \
	entropy.cpp \
	tests.cpp \

OBJS=$(subst .cpp,.o,$(SRCS))

all: cwordle

cwordle: $(OBJS)
	$(CXX) $(LDFLAGS) -o cwordle $(OBJS) $(LDLIBS)

clean:
	$(RM) $(OBJS) cwordle

