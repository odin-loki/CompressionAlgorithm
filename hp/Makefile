# hp — Cypha Hutter Prize core
CXX      ?= g++
XSIMD    ?= 1
BIN      ?= build
XSIMD_FLAGS :=
ifeq ($(XSIMD),1)
  XSIMD_FLAGS := -DHP_XSIMD=1 -msse4.1 -Ithird_party/xsimd/include
endif
CXXFLAGS ?= -std=c++17 -O2 -Wall -Iinclude $(XSIMD_FLAGS)

all: $(BIN)/hp $(BIN)/dump_experts $(BIN)/bench

$(BIN):
	mkdir -p $(BIN)

$(BIN)/hp: src/main.cpp $(wildcard include/hp/*.hpp) | $(BIN)
	$(CXX) $(CXXFLAGS) -o $@ $<

$(BIN)/dump_experts: tools/dump_experts.cpp $(wildcard include/hp/*.hpp) | $(BIN)
	$(CXX) $(CXXFLAGS) -o $@ $<

$(BIN)/bench: tools/bench.cpp $(wildcard include/hp/*.hpp) | $(BIN)
	$(CXX) $(CXXFLAGS) -o $@ $<

test: all
	bash test/no_float.sh
	HP=$(BIN)/hp bash test/roundtrip.sh $(FILES)

determinism: 
	bash test/determinism.sh $(FILE)

clean:
	rm -rf $(BIN)

.PHONY: all test determinism clean
