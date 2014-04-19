##############################################################################
# Makefile: lab5 makefile                                          
##############################################################################
#
#
	
SUBDIRS	=  src

# --- Compilation

all:subdirs

subdirs:
	@set -e ; for d in $(SUBDIRS); do	\
	  $(MAKE) -C $$d ;			\
	done

# --- Maintenance targets

clean:
	@set -e ; for d in $(SUBDIRS); do	\
	  $(MAKE) -C $$d $@ ;			\
	done
