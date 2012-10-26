default: all

include /usr/share/phantom/library.mk

$(eval $(call LIBRARY,pi))
$(eval $(call LIBRARY,lightning))

include /usr/share/phantom/module.mk

$(eval $(call MODULE,io_blob_sender,,pi lightning,))

FIXINC = -isystem . -isystem /usr/include/pd/fixinclude

include /usr/share/phantom/opts.mk

all: $(targets)

clean:; @rm -f $(targets) $(tmps) deps/*.d

.PHONY: default all clean

