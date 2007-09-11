.PHONY: test
test: test_lib


.PHONY: test_lib
test_lib:
	@cd test_lib && $(MAKE) test
	
	
.PHONY: clean
clean:
	@rm -f *~
	@cd test_lib && $(MAKE) $@
	