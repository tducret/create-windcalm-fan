ESPHOME ?= .venv/bin/esphome
CXX ?= c++

.PHONY: test config compile
test:
	mkdir -p .build
	$(CXX) -std=c++17 -Wall -Wextra -Werror -fsanitize=address,undefined -I. tests/protocol_test.cpp -o .build/protocol_test
	.build/protocol_test

config:
	$(ESPHOME) config examples/packages-local.yaml

compile:
	$(ESPHOME) compile examples/packages-local.yaml
