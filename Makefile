default: all

include /usr/share/phantom/library.mk

#$(eval $(call LIBRARY,paxos))
$(eval $(call LIBRARY,vars))

include /usr/share/phantom/module.mk

#$(eval $(call MODULE,io_learner,,paxos))
#$(eval $(call MODULE,io_datagram))
#$(eval $(call MODULE,io_datagram/ipv4))
#$(eval $(call MODULE,io_datagram/handler_echo_log))
#$(eval $(call MODULE,io_datagram/handler_learner,,,protobuf))
#$(eval $(call MODULE,io_datagram/multicast_ipv4))
$(eval $(call MODULE,io_zookeeper,,vars,zookeeper_mt))

FIXINC = -isystem . -isystem /usr/include/pd/fixinclude -I /home/skywalker/czk/include/c-client-src

include /usr/share/phantom/opts.mk

all: $(targets)

clean:; @rm -f $(targets) $(tmps) deps/*.d

.PHONY: default all clean

