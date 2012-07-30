default: all

include /usr/share/phantom/library.mk

$(eval $(call LIBRARY,paxos))

include /usr/share/phantom/opts.mk

all: $(targets)

clean:; @rm -f $(targets) $(tmps) deps/*.d

.PHONY: default all clean

