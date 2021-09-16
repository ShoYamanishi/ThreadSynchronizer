.DEFAULT_GOAL := all

.PHONY: all
.PHONY: clean
.PHONY: test

SAMPLE_DIR = samples
SAMPLE_SRC_FILES = sample_01.cpp sample_02.cpp sample_03.cpp \
	binary_oscillator.cpp \
	cycle_scheduler_infinite.cpp \
	cycle_scheduler_finite.cpp \
	parallel_scheduler.cpp \
	parallel_scheduler_with_mid_sync.cpp

TEST_DIR = test
TEST_SRC = test_cpu_parallel_processing.cpp

APPLE_SDK        = -L/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX.sdk
APPLE_FRAMEWORKS = -framework Foundation

CC         = clang++
LD         = clang++
CD         = cd
RMR        = rm -fr
RM         = rm -f

DIR_GUARD  = @mkdir -p $(@D)

OBJ_DIR    = objs
BIN_DIR    = bin

CCFLAGS     = -Wall -std=c++20 -stdlib=libc++ -O3 -I.
LDFLAGS     = $(APPLE_SDK) $(APPLE_FRAMEWORKS)

SAMPLE_SRCS = $(patsubst %,$(OBJ_DIR)/%,$(SAMPLE_SRC_FILES))
SAMPLE_OBJS = $(patsubst %,$(OBJ_DIR)/%,$(subst .cpp,.o,$(SAMPLE_SRC_FILES)))
SAMPLE_BINS = $(patsubst %,$(BIN_DIR)/%,$(subst .cpp,,$(SAMPLE_SRC_FILES)))

TEST_OBJS   = $(patsubst %,$(OBJ_DIR)/%,$(subst .cpp,.o,$(TEST_SRC)))
TEST_BIN    = $(BIN_DIR)/$(basename $(TEST_SRC))

$(OBJ_DIR)/%.o: $(SAMPLE_DIR)/%.cpp
	$(DIR_GUARD)
	$(CC) $(CCFLAGS) $(CC_INC) -c $< -o $@

$(BIN_DIR)/%: $(OBJ_DIR)/%.o
	$(DIR_GUARD)
	$(LD) $(LDFLAGS) $^ -o $@

$(TEST_OBJS): $(TEST_DIR)/$(TEST_SRC)
	$(DIR_GUARD)
	$(CC) $(CCFLAGS) $(CC_INC) -c $< -o $@

$(TEST_BIN): $(TEST_OBJS)
	$(DIR_GUARD)
	$(LD) $(LDFLAGS) $^ -o $@

test: $(TEST_BIN)
	$(CD) $(BIN_DIR); $(subst $(BIN_DIR)/,./,$(TEST_BIN))

all: $(SAMPLE_BINS) $(TEST_BIN)

clean:
	-$(RMR) $(OBJ_DIR) $(BIN_DIR)

