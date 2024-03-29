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

HDRS = \
	commands.h \
	counter_map.h \
	cwordle.h \
	dictionary.h \
	entropy.h \
	formatted.h \
	partial_sorted_list.h \
	random.h \
	styled_text.h \
	tests.h \
	timers.h \
	timing_reporter.h \
	types.h \
	wordle_word.h \
	word_list.h \

OBJS=$(subst .cpp,.o,$(SRCS))
ROBJS=$(subst vocabulary.o,,$(OBJS))

all: cwordle

cwordle: $(OBJS)
	$(CXX) $(LDFLAGS) -o cwordle $(OBJS) $(LDLIBS)

$(ROBJS): $(HDRS)

clean:
	$(RM) $(OBJS) cwordle

