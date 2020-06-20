# Makefile to build class 'pdfirmata' for Pure Data.
# Needs Makefile.pdlibbuilder as helper makefile for platform-dependent build
# settings and rules.

# library name
lib.name = pdfirmata

# input source file (class name == source file basename)
class.sources = pdfirmata.c

# all extra files to be included in binary distribution of the library
datafiles = pdfirmata-help.pd README.md LICENSE

PDINCLUDEDIR=pure-data/src/
PDBINDIR=bin/

# include Makefile.pdlibbuilder from submodule directory 'pd-lib-builder'
PDLIBBUILDER_DIR=pd-lib-builder/
include $(PDLIBBUILDER_DIR)/Makefile.pdlibbuilder