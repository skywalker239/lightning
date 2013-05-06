default: all

include /usr/share/phantom/library.mk

$(eval $(call LIBRARY,pi))
$(eval $(call LIBRARY,lightning))
$(eval $(call LIBRARY,zookeeper))
$(eval $(call LIBRARY,zk_vars))

include /usr/share/phantom/module.mk

include gtest.mk

#$(eval $(call MODULE,io_zclient,,,))
#$(eval $(call MODULE,io_toy_zk_client,,pi lightning,))
#$(eval $(call MODULE,io_zhandle,,,zookeeper_mt))
#$(eval $(call MODULE,io_zcluster_status,,zookeeper,))
#$(eval $(call MODULE,io_zconf,,pi lightning,))
#$(eval $(call MODULE,io_zmaster,,lightning,))
#$(eval $(call MODULE,io_transport_config,,pi lightning,))

$(eval $(call MODULE,io_blob_sender,,pi lightning,))
$(eval $(call MODULE,io_blob_receiver,,pi lightning,))

#$(eval $(call MODULE,io_toy_zk_client,,pi lightning,))
$(eval $(call MODULE,io_zhandle,,,zookeeper_mt))
$(eval $(call MODULE,io_zclient,,,))
#$(eval $(call MODULE,io_zcluster_status,,zookeeper,))
#$(eval $(call MODULE,io_zconf,,pi lightning,))
#$(eval $(call MODULE,io_zmaster,,lightning,))
#$(eval $(call MODULE,io_transport_config,,pi lightning,))

$(eval $(call MODULE,io_guid,,pi lightning,))
$(eval $(call MODULE,io_var_store,,pi lightning,))

$(eval $(call MODULE,io_ring_sender,,pi lightning,))
$(eval $(call MODULE,ring_handler,,pi lightning,))

$(eval $(call MODULE,io_acceptor_store,,pi lightning,))
$(eval $(call MODULE,io_proposer_pool,,pi lightning,))

$(eval $(call MODULE,io_paxos_executor,,pi lightning,))
$(eval $(call MODULE,io_phase1_batch_executor,,pi lightning,))
$(eval $(call MODULE,io_phase1_executor,,pi lightning,))
$(eval $(call MODULE,io_phase2_executor,,pi lightning,))

$(eval $(call MODULE,io_lightning_static_server,,pi lightning,))

# test modules
$(eval $(call MODULE,io_gtest_runner,,gtest,))
$(eval $(call MODULE,test_gtests,,,))

$(eval $(call MODULE,test_pd_lightning,,pi lightning,))
$(eval $(call MODULE,test_pd_intrusive,,pi lightning,))
$(eval $(call MODULE,test_ring,,pi lightning,))
$(eval $(call MODULE,test_blob_multicast,,pi lightning,))
$(eval $(call MODULE,test_executors,,pi lightning,))
$(eval $(call MODULE,test_paxos_structures,,pi lightning,))

FIXINC = -isystem . -isystem /usr/include/pd/fixinclude -isystem ./contrib/gmock

include /usr/share/phantom/opts.mk

all: $(targets)

clean:; @rm -f $(targets) $(tmps) deps/*.d

.PHONY: default all clean

