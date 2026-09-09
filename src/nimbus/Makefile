# Project Name
TARGET = nimbus

# Sources
CPP_SOURCES = \
nimbus.cpp \
../../lib/loewy.cpp \
dsp/correlator.cpp \
dsp/granular_processor.cpp \
dsp/pvoc/frame_transformation.cpp \
dsp/pvoc/phase_vocoder.cpp \
dsp/pvoc/stft.cpp \
resources.cpp

# Library Locations
LIBDAISY_DIR = ../../lib/libDaisy
DAISYSP_DIR = ../../lib/DaisySP

C_INCLUDES += -I. -Idsp -Idsp/pvoc -Idsp/fx

# Core location, and generic Makefile.
SYSTEM_FILES_DIR = $(LIBDAISY_DIR)/core
include $(SYSTEM_FILES_DIR)/Makefile
