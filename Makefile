CXX=g++
RM=rm -f
OBJDIR=obj
#CXXFLAGS=-g -mavx -mavx2 -mavx512f -mavx512cd -mavx512vl -std=c++23 -O0
CXXFLAGS=-g -march=native -std=c++23 -Ipistache/include  -Wno-deprecated -O0
LDFLAGS=-g 
LDLIBS=-lboost_program_options -lboost_filesystem -lboost_system -Lpistache/build/src -l:libpistache.a

XX1:=$(shell mkdir -p $(OBJDIR))

COMMON_SRCS= \
	cwordle.cpp \
	globals.cpp \
	styled_text.cpp \
	wordle_word.cpp \
	word_list.cpp \
	dictionary.cpp \
	options.cpp \
	random.cpp \
	vocabulary.cpp \
	entropy.cpp \

CLI_SRCS= \
	main.cpp \
	commands.cpp \
	tests.cpp

WEB_SRCS= \
	web_server.cpp \

HDRS = \
	avx.h \
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

CLI_ALL= $(CLI_SRCS) $(COMMON_SRCS)
WEB_ALL= $(WEB_SRCS) $(COMMON_SRCS)

CLI_OBJS=$(addprefix $(OBJDIR)/,$(subst .cpp,.o,$(CLI_ALL)))
WEB_OBJS=$(addprefix $(OBJDIR)/,$(subst .cpp,.o,$(WEB_ALL)))

all: cwordle

$(OBJDIR)/%.o: %.cpp
	$(CXX) $(CXXFLAGS) -g -c $< -o $@

cwordle: $(CLI_OBJS)
	$(CXX) $(LDFLAGS) -o cwordle $(CLI_OBJS) $(LDLIBS)

$(CLI_OBJS): $(HDRS)

web_server: $(WEB_OBJS)
	$(CXX) $(LDFLAGS) \
	-o cwordle_web_server $(WEB_OBJS) $(LDLIBS)

$(WEB_OBJS): $(HDRS)

