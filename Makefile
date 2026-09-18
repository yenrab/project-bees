# Project BEES root Makefile. The shim is a Silica library; there is nothing to build at the root
# except the trial tree, which is how BEES is verified (roadmap §4.9, R0.1).
BEES_ROOT := $(abspath $(dir $(lastword $(MAKEFILE_LIST))))
include $(BEES_ROOT)/tools/silica.mk

.PHONY: integrate record-golden build clean help

integrate: check-compiler
	@$(MAKE) -C trials integrate

record-golden: check-compiler
	@$(MAKE) -C trials record-golden

build: check-compiler
	@$(MAKE) -C trials build

clean:
	@$(MAKE) -C trials clean

help:
	@echo "make integrate      build, run and diff every trial leaf against its goldens"
	@echo "make record-golden  re-record every golden (after a deliberate change only)"
	@echo "make build | clean"
	@echo "compiler: $(SILICA_COMPILER)"
