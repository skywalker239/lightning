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
$(eval $(call MODULE,io_toy_zk_client,,vars))

FIXINC = -isystem . -isystem /usr/include/pd/fixinclude -I /home/skywalker/czk/include/c-client-src

OPT = 0 -g3
include /usr/share/phantom/opts.mk

all: $(targets)

clean:; @rm -f $(targets) $(tmps) deps/*.d

.PHONY: default all clean

