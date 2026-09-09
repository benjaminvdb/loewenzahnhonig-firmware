# Shared build rules for Nimbus and its variants.
#
# A variant folder's Makefile sets TARGET, NIMBUS_DIR (the folder containing
# this file) and NIMBUS_VARIANT (the NIMBUS_VARIANT_* define that selects the
# configuration in nimbus.cpp), then includes this file.

NIMBUS_DIR ?= .

# Sources
CPP_SOURCES = \
$(NIMBUS_DIR)/nimbus.cpp \
$(NIMBUS_DIR)/../../lib/loewy.cpp \
$(NIMBUS_DIR)/dsp/correlator.cpp \
$(NIMBUS_DIR)/dsp/granular_processor.cpp \
$(NIMBUS_DIR)/dsp/pvoc/frame_transformation.cpp \
$(NIMBUS_DIR)/dsp/pvoc/phase_vocoder.cpp \
$(NIMBUS_DIR)/dsp/pvoc/stft.cpp \
$(NIMBUS_DIR)/resources.cpp

ifneq ($(NIMBUS_VARIANT),)
C_DEFS += -D$(NIMBUS_VARIANT)
endif

C_INCLUDES += -I$(NIMBUS_DIR) -I$(NIMBUS_DIR)/dsp -I$(NIMBUS_DIR)/dsp/pvoc -I$(NIMBUS_DIR)/dsp/fx

# Library Locations
LIBDAISY_DIR = $(NIMBUS_DIR)/../../lib/libDaisy
DAISYSP_DIR = $(NIMBUS_DIR)/../../lib/DaisySP

# Core location, and generic Makefile.
SYSTEM_FILES_DIR = $(LIBDAISY_DIR)/core
include $(SYSTEM_FILES_DIR)/Makefile
