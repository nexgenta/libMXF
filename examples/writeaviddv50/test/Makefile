WRITEAVIDDV50_PATH = ../
LIBMXF_TEST_PATH = ../../../test


.PHONY: test
test: writeaviddv50
	@$(LIBMXF_TEST_PATH)/run_test.sh writeaviddv50 \
		"./writeaviddv50 input.dv writeaviddv50.mxf" $(LIBMXF_TEST_PATH)
		
	
.PHONY: create
create: writeaviddv50
	@env LIBMXF_TEST_PATH=$(LIBMXF_TEST_PATH) $(LIBMXF_TEST_PATH)/create_test_set.sh writeaviddv50 \
		"./writeaviddv50 input.dv writeaviddv50.mxf" "./writeaviddv50 input.dv writeaviddv50.mxf_2" 
	
	
.PHONY: writeaviddv50
writeaviddv50:
	@cd $(WRITEAVIDDV50_PATH) && $(MAKE) writeaviddv50
	@mv $(WRITEAVIDDV50_PATH)/writeaviddv50 .
	
	
.PHONY: clean
clean:
	@rm -f *~
	@rm -f writeaviddv50
	@rm -f *results_std*.txt
	
