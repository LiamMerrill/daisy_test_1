# Project Name
TARGET = pod_sample_test

# Sources
CPP_SOURCES = pod_sample_test.cpp

# Library Locations
LIBDAISY_DIR = ../libDaisy

# Core location, and generic Makefile.
SYSTEM_FILES_DIR = $(LIBDAISY_DIR)/core
include $(SYSTEM_FILES_DIR)/Makefile