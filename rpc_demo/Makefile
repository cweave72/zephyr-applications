include ../../common.mk

PROTO_LIB_DEST := $(CURDIR)/python/proto

proto:
	@echo "Building common proto python bindings."
	$(MAKE) -C $(COMMON_PROTO_BASE) DST=$(PROTO_LIB_DEST) proto

cleanproto:
	@echo "Cleaning $(PROTO_LIB_DEST)."
	rm -rf $(PROTO_LIB_DEST)
