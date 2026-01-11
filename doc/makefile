#
# Documentation ONLY, Copyright (c) 2026 AESilky
#
# ############################################################################
#
# Makefile to create:
#  * The manuals (pdf from the adoc)
#
# The Makefile is for GNU Make. On Windows, it is expected that Cygwin is used.
# On Mac and Linux, it is expected that Bash shell is available. The path to
# the shell is set using the *SHELL* variable.
#

# Cygwin path conversions for use when programs need a full path.
ifdef COMSPEC
    cygpath-mixed			= $(shell cygpath -m "$1")
    cygpath-unix			= $(shell cygpath -u "$1")
    drive-letter-to-slash	= /$(subst :,,$1)
    EXEC_EXT				:= .exe
else
    cygpath-mixed			= $1
    cygpath-unix			= $1
    drive-letter-to-slash	= $1
    EXEC_EXT				:= .bin
endif

AWK			?= awk
CP			?= cp
CP_RECURSE		?= $(CP) -rup
CP_TOFILE		?= $(CP) -Tup
CP_UPDATE		?= $(CP) -up
GREP			?= grep
PDFGEN			?= asciidoctor-pdf
PR			?= pr
SHELL			:= /bin/bash
SORT			?= sort

# ### Documentation Section                                          ###

DOC_DIR			?= ./
#
MANUAL_SETTINGS		:= $(DOC_DIR)man_settings.adoc
DOC_THEME		:= $(DOC_DIR)Unidot-theme.yml
USER_MANUAL_BASE	:= UASM_ULINK-User-Manual
USER_MANUAL_SRC		:= $(DOC_DIR)$(USER_MANUAL_BASE).adoc
USER_MANUAL		:= $(DOC_DIR)$(USER_MANUAL_BASE).pdf

# General purpose macros
#
# $(call kill-program,awk-pattern)
define kill-program
	@ $(PS) $(PS_FLAGS) |										\
	$(AWK) 'BEGIN	{ FIELDWIDTHS = $(PS_FIELDS) }				\
		/$1/	{												\
						print "Killing " $$3;					\
						system( "$(KILL) $(KILL_FLAGS) " $$1 )	\
					}'
endef

# Documentation rules
%.pdf: %.adoc
	$(PDFGEN) $<


# help (the default target) - Print a list of all targets in this makefile
.PHONY: help
help:
	@$(MAKE_COMMAND) --print-data-base --question no-such-target | \
	$(GREP) -v -e '^no-such-target' -e '^makefile' | \
	$(AWK) '/^[^.%][-A-Za-z0-9_]*:/ { print substr($$1, 1, length($$1)-1) }' | \
	$(SORT) | \
	$(PR) --omit-pagination --width=132 --columns=4

# Currently 'all' is just the documentation. It will become everything (binaries, docs, installers)
# once the kinks are ironed out of the binaries.
#
.PHONY: all
all: docs

.PHONY: docs
docs: $(USER_MANUAL)

# #############################################################################
#
# True targets
#
# #############################################################################

$(USER_MANUAL): $(USER_MANUAL_SRC)  $(MANUAL_SETTINGS) $(DOC_THEME)
	$(PDFGEN) $<

