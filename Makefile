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

# Add build directory
BUILD_DIR = build

# Update binary and object file locations
OBJS = $(BUILD_DIR)/main.o $(BUILD_DIR)/cwordle.o $(BUILD_DIR)/web_server.o $(BUILD_DIR)/dictionary.o $(BUILD_DIR)/entropy.o $(BUILD_DIR)/random.o $(BUILD_DIR)/styled_text.o $(BUILD_DIR)/vocabulary.o $(BUILD_DIR)/word_list.o $(BUILD_DIR)/wordle_word.o

# Update targets to use build directory
$(BUILD_DIR)/web_server: $(OBJS)
	$(CXX) $(CXXFLAGS) -g $(OBJS) $(HOME)/big/misc/cwordle/pistache/build/src/libpistache.a -lpthread -ldl -lboost_program_options -lboost_filesystem -lboost_system -o $(BUILD_DIR)/web_server

$(BUILD_DIR)/%.o: %.cpp
	@mkdir -p $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -g -c $< -o $@

# Update clean rule
clean:
	$(RM) $(OBJDIR)
	$(RM) cwordle cwordle_web_server

#old_web_server: cwordle.cpp web_server.cpp dictionary.cpp entropy.cpp random.cpp styled_text.cpp vocabulary.cpp word_list.cpp wordle_word.cpp
#	g++ -std=c++23 -O0 -mavx -mavx2 -mavx512f -mavx512cd -mavx512vl -mfma -I. -Wno-deprecated -I$(HOME)/big/misc/cwordle/pistache/include \
#	  globals.cpp options.cpp cwordle.cpp web_server.cpp dictionary.cpp entropy.cpp random.cpp styled_text.cpp vocabulary.cpp word_list.cpp wordle_word.cpp \
	  $(HOME)/big/misc/cwordle/pistache/build/src/libpistache.a \
	  -lpthread -ldl -lboost_program_options -lboost_filesystem -lboost_system -g -o cwordle_web_server

