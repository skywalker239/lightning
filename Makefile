default: all

include /usr/share/phantom/library.mk

$(eval $(call LIBRARY,pi))

include /usr/share/phantom/module.mk

$(eval $(call MODULE,io_blob_sender,,pi,))

FIXINC = -isystem . -isystem /usr/include/pd/fixinclude

include /usr/share/phantom/opts.mk

all: $(targets)

clean:; @rm -f $(targets) $(tmps) deps/*.d

.PHONY: default all clean

