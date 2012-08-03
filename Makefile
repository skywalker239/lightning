default: all

include /usr/share/phantom/library.mk

$(eval $(call LIBRARY,paxos))

include /usr/share/phantom/module.mk

$(eval $(call MODULE,io_datagram))
$(eval $(call MODULE,io_datagram/ipv4))
$(eval $(call MODULE,io_datagram/handler_echo_log))

include /usr/share/phantom/opts.mk

all: $(targets)

clean:; @rm -f $(targets) $(tmps) deps/*.d

.PHONY: default all clean

