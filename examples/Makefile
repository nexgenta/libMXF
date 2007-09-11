
.PHONY:all
all: writeaviddv50 writeavidmxf



.PHONY: writeaviddv50
writeaviddv50:
	@cd writeaviddv50 && $(MAKE) all


.PHONY: writeavidmxf
writeavidmxf:
	@cd writeavidmxf && $(MAKE) all
	
	
.PHONY: transfertop2
transfertop2:
	@cd transfertop2 && $(MAKE) all
	
	
.PHONY: archive
archive:
	@cd archive && $(MAKE) all
	
	
.PHONY: reader
reader:
	@cd reader && $(MAKE) all
	
	
.PHONY: clean
clean:
	@rm -f *~
	@cd writeaviddv50 && $(MAKE) $@
	@cd writeavidmxf && $(MAKE) $@
	@cd transfertop2 && $(MAKE) $@
	@cd archive && $(MAKE) $@
	@cd reader && $(MAKE) $@
	
	
	