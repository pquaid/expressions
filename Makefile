
LIBSRC := textsource.cc tokenizer.cc expression.cc
LIBOBJ := $(LIBSRC:.cc=.o)
LIBDEP := $(LIBOBJ:.o=.d)

TSTSRC := tokenizer_test.cc textsource_test.cc expression_test.cc
TSTOBJ := $(TSTSRC:.cc=.o)
TSTDEP := $(TSTOBJ:.o=.d)
TSTBIN := $(TSTSRC:.cc=)
CHECKBIN := $(foreach bin,$(TSTBIN),check_$(bin))

LIB := libexpression.a
BINSRC := expr.cc
BINOBJ := $(BINSRC:.cc=.o)
BINDEP := $(BINOBJ:.o=.d)
BIN    := $(BINSRC:.cc=)

CXXFLAGS := -std=c++11 -Wall
LDFLAGS := -L. -lexpression

GTEST_DIR := /usr/local
GTEST_CXXFLAGS := $(CXXFLAGS) -g -isystem $(GTEST_DIR)/include -I$(GEST_DIR) -pthread
GTEST_LDFLAGS := -pthread -L$(GTEST_DIR)/lib -lgtest_main -lgtest

TESTOUT := $(shell /bin/mktemp -u)

all: lib bin test check

lib: $(LIB)

bin: $(BIN)

test: $(TSTBIN)

$(LIB): $(LIBOBJ)
	ar rcsu $(LIB) $(LIBOBJ)

$(BIN): %: %.o $(LIB)
	$(CXX) $< -o $@ $(LDFLAGS)

$(TSTBIN): %: %.o $(LIB)
	$(CXX) $< -o $@ $(LDFLAGS) $(GTEST_LDFLAGS)

check: $(TSTBIN) $(CHECKBIN)

$(CHECKBIN): check_%: %
	@(./$< >$(TESTOUT) && echo "PASS:" $< && rm $(TESTOUT)) || \
	 (echo "FAIL:" $< ; cat $(TESTOUT); rm $(TESTOUT); exit 1)

$(TSTOBJ): %.o: %.cc
	$(CXX) -c -MMD -MP $(GTEST_CXXFLAGS) $< -o $@

%.o: %.cc
	$(CXX) -c -MMD -MP $(CXXFLAGS) $< -o $@

clean:
	-$(RM) $(LIB) $(LIBOBJ) $(LIBDEP)
	-$(RM) $(TSTBIN) $(TSTOBJ) $(TSTDEP)
	-$(RM) $(BIN) $(BINOBJ) $(BINDEP)
	-$(RM) *~

-include $(LIBDEP) $(TSTDEP) $(BINDEP)


