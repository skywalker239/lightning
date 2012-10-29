default: all

include /usr/share/phantom/library.mk

$(eval $(call LIBRARY,pi))
$(eval $(call LIBRARY,lightning))
$(eval $(call LIBRARY,zookeeper))

include /usr/share/phantom/module.mk

$(eval $(call MODULE,io_blob_sender,,pi lightning,))
$(eval $(call MODULE,io_zclient,,,))
$(eval $(call MODULE,io_toy_zk_client,,lightning,))
$(eval $(call MODULE,io_zhandle,,,zookeeper_mt))
$(eval $(call MODULE,io_zcluster_status,,zookeeper,))
$(eval $(call MODULE,io_zconf,,lightning,))
$(eval $(call MODULE,io_zmaster,,lightning,))

FIXINC = -isystem . -isystem /usr/include/pd/fixinclude

include /usr/share/phantom/opts.mk

all: $(targets)

clean:; @rm -f $(targets) $(tmps) deps/*.d

.PHONY: default all clean

