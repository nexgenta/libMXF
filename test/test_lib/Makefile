LIBMXF_PATH = ../../lib
LIBMXF_TEST_PATH = ..

CC = gcc -g -O2 -D_FILE_OFFSET_BITS=64 -D_LARGEFILE_SOURCE -D_LARGEFILE64_SOURCE
INCLUDES = -I. -I$(LIBMXF_PATH)/include


.PHONY: test
test: testfile testpartition testprimer testindextable testdatamodel \
	testessencecontainer testheadermetadata
		
.PHONY: testfile
testfile: libMXF.a test_file
	@$(LIBMXF_TEST_PATH)/run_test_nodiff.sh file \
		"./test_file file.mxf" $(LIBMXF_TEST_PATH)
		
.PHONY: testpartition
testpartition: libMXF.a test_partition
	@$(LIBMXF_TEST_PATH)/run_test.sh partition \
		"./test_partition partition.mxf" $(LIBMXF_TEST_PATH)

.PHONY: testprimer
testprimer: libMXF.a test_primer
	@$(LIBMXF_TEST_PATH)/run_test.sh primer \
		"./test_primer primer.mxf" $(LIBMXF_TEST_PATH)
		
.PHONY: testindextable
testindextable: libMXF.a test_indextable
	@$(LIBMXF_TEST_PATH)/run_test.sh indextable \
		"./test_indextable indextable.mxf" $(LIBMXF_TEST_PATH)
		
.PHONY: testdatamodel
testdatamodel: libMXF.a test_datamodel
	@$(LIBMXF_TEST_PATH)/run_test_nodiff.sh datamodel \
		"./test_datamodel" $(LIBMXF_TEST_PATH)
		
.PHONY: testessencecontainer
testessencecontainer: libMXF.a test_essencecontainer
	@$(LIBMXF_TEST_PATH)/run_test.sh essencecontainer \
		"./test_essencecontainer essencecontainer.mxf" $(LIBMXF_TEST_PATH)

.PHONY: testheadermetadata
testheadermetadata: libMXF.a test_headermetadata
	@$(LIBMXF_TEST_PATH)/run_test.sh headermetadata \
		"./test_headermetadata headermetadata.mxf" $(LIBMXF_TEST_PATH)

	
	
.PHONY: create
create: createfile createpartition createprimer createindextable createdatamodel \
	createessencecontainer createheadermetadata

.PHONY: createfile
createfile:
	
.PHONY: createpartition
createpartition: libMXF.a test_partition
	@env LIBMXF_TEST_PATH=$(LIBMXF_TEST_PATH) $(LIBMXF_TEST_PATH)/create_test_set.sh partition \
		"./test_partition partition.mxf" "./test_partition partition.mxf_2"

.PHONY: createprimer
createprimer: libMXF.a test_primer
	@env LIBMXF_TEST_PATH=$(LIBMXF_TEST_PATH) $(LIBMXF_TEST_PATH)/create_test_set.sh primer \
		"./test_primer primer.mxf" "./test_primer primer.mxf_2"

.PHONY: createindextable
createindextable: libMXF.a test_indextable
	@env LIBMXF_TEST_PATH=$(LIBMXF_TEST_PATH) $(LIBMXF_TEST_PATH)/create_test_set.sh indextable \
		"./test_indextable indextable.mxf" "./test_indextable indextable.mxf_2"

.PHONY: createdatamodel
createdatamodel:
	
.PHONY: createessencecontainer
createessencecontainer: libMXF.a test_essencecontainer
	@env LIBMXF_TEST_PATH=$(LIBMXF_TEST_PATH) $(LIBMXF_TEST_PATH)/create_test_set.sh essencecontainer \
		"./test_essencecontainer essencecontainer.mxf" "./test_essencecontainer essencecontainer.mxf_2"

.PHONY: createheadermetadata
createheadermetadata: libMXF.a test_headermetadata
	@env LIBMXF_TEST_PATH=$(LIBMXF_TEST_PATH) $(LIBMXF_TEST_PATH)/create_test_set.sh headermetadata \
		"./test_headermetadata headermetadata.mxf" "./test_headermetadata headermetadata.mxf_2"

	
.PHONY: libMXF.a
libMXF.a:
	@cd $(LIBMXF_PATH) && $(MAKE) $@


test_file: libMXF.a test_file.o
	$(CC) test_file.o -L$(LIBMXF_PATH) -lMXF -luuid -o test_file

test_file.o: test_file.c $(LIBMXF_PATH)/include/mxf/mxf.h
	$(CC) -c $(INCLUDES) test_file.c
	
test_partition: libMXF.a test_partition.o
	$(CC) test_partition.o -L$(LIBMXF_PATH) -lMXF -luuid -o test_partition

test_partition.o: test_partition.c $(LIBMXF_PATH)/include/mxf/mxf.h
	$(CC) -c $(INCLUDES) test_partition.c
	
test_primer: libMXF.a test_primer.o
	$(CC) test_primer.o -L$(LIBMXF_PATH) -lMXF -luuid -o test_primer

test_primer.o: test_primer.c $(LIBMXF_PATH)/include/mxf/mxf.h
	$(CC) -c $(INCLUDES) test_primer.c
	
test_indextable: libMXF.a test_indextable.o
	$(CC) test_indextable.o -L$(LIBMXF_PATH) -lMXF -luuid -o test_indextable

test_indextable.o: test_indextable.c $(LIBMXF_PATH)/include/mxf/mxf.h
	$(CC) -c $(INCLUDES) test_indextable.c
	
test_datamodel: libMXF.a test_datamodel.o
	$(CC) test_datamodel.o -L$(LIBMXF_PATH) -lMXF -luuid -o test_datamodel

test_datamodel.o: test_datamodel.c $(LIBMXF_PATH)/include/mxf/mxf.h
	$(CC) -c $(INCLUDES) test_datamodel.c
	
test_essencecontainer: libMXF.a test_essencecontainer.o
	$(CC) test_essencecontainer.o -L$(LIBMXF_PATH) -lMXF -luuid -o test_essencecontainer

test_essencecontainer.o: test_essencecontainer.c $(LIBMXF_PATH)/include/mxf/mxf.h
	$(CC) -c $(INCLUDES) test_essencecontainer.c
	
test_headermetadata: libMXF.a test_headermetadata.o
	$(CC) test_headermetadata.o -L$(LIBMXF_PATH) -lMXF -luuid -o test_headermetadata

test_headermetadata.o: test_headermetadata.c $(LIBMXF_PATH)/include/mxf/mxf.h
	$(CC) -c $(INCLUDES) test_headermetadata.c
	

	
	
.PHONY: clean
clean:
	@rm -f *~ *.o 
	@rm -f test_file test_partition test_primer test_indextable test_datamodel test_essencecontainer test_headermetadata
	@rm -f *results_std*.txt

