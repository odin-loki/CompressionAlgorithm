# hp — Cypha Hutter Prize core
CXX      ?= g++
CXXFLAGS ?= -std=c++17 -O2 -Wall -Iinclude
BIN      ?= build

all: $(BIN)/hp $(BIN)/dump_experts

$(BIN):
	mkdir -p $(BIN)

$(BIN)/hp: src/main.cpp $(wildcard include/hp/*.hpp) | $(BIN)
	$(CXX) $(CXXFLAGS) -o $@ $<

$(BIN)/dump_experts: tools/dump_experts.cpp $(wildcard include/hp/*.hpp) | $(BIN)
	$(CXX) $(CXXFLAGS) -o $@ $<

test: all
	bash test/no_float.sh
	HP=$(BIN)/hp bash test/roundtrip.sh $(FILES)

determinism: 
	bash test/determinism.sh $(FILE)

clean:
	rm -rf $(BIN)

.PHONY: all test determinism clean
