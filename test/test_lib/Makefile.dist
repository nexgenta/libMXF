#
# $Id: Makefile,v 1.2 2008/05/07 15:22:32 philipn Exp $
#
# Makefile for building and running tests
#
# Copyright (C) 2006  BBC Research, Philip de Nier <philipn@users.sourceforge.net>
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
# 02110-1301, USA.
#
TOPLEVEL = ../..
include $(TOPLEVEL)/vars.mk

LIBMXF_TEST_PATH = ..

.PHONY: all
all: test_file test_partition test_primer test_indextable test_datamodel \
       test_essencecontainer test_headermetadata

.PHONY: check
check: testfile testpartition testprimer testindextable testdatamodel \
	testessencecontainer testheadermetadata

.PHONY: testfile
testfile: test_file
	@$(LIBMXF_TEST_PATH)/run_test_nodiff.sh file \
		"./test_file file.mxf" $(LIBMXF_TEST_PATH)

.PHONY: testpartition
testpartition: test_partition
	@$(LIBMXF_TEST_PATH)/run_test.sh partition \
		"./test_partition partition.mxf" $(LIBMXF_TEST_PATH)

.PHONY: testprimer
testprimer: test_primer
	@$(LIBMXF_TEST_PATH)/run_test.sh primer \
		"./test_primer primer.mxf" $(LIBMXF_TEST_PATH)

.PHONY: testindextable
testindextable: test_indextable
	@$(LIBMXF_TEST_PATH)/run_test.sh indextable \
		"./test_indextable indextable.mxf" $(LIBMXF_TEST_PATH)

.PHONY: testdatamodel
testdatamodel: test_datamodel
	@$(LIBMXF_TEST_PATH)/run_test_nodiff.sh datamodel \
		"./test_datamodel" $(LIBMXF_TEST_PATH)

.PHONY: testessencecontainer
testessencecontainer: test_essencecontainer
	@$(LIBMXF_TEST_PATH)/run_test.sh essencecontainer \
		"./test_essencecontainer essencecontainer.mxf" $(LIBMXF_TEST_PATH)

.PHONY: testheadermetadata
testheadermetadata: test_headermetadata
	@$(LIBMXF_TEST_PATH)/run_test.sh headermetadata \
		"./test_headermetadata headermetadata.mxf" $(LIBMXF_TEST_PATH)



.PHONY: create
create: createfile createpartition createprimer createindextable createdatamodel \
	createessencecontainer createheadermetadata

.PHONY: createfile
createfile:

.PHONY: createpartition
createpartition: test_partition
	@env LIBMXF_TEST_PATH=$(LIBMXF_TEST_PATH) $(LIBMXF_TEST_PATH)/create_test_set.sh partition \
		"./test_partition partition.mxf" "./test_partition partition.mxf_2"

.PHONY: createprimer
createprimer: test_primer
	@env LIBMXF_TEST_PATH=$(LIBMXF_TEST_PATH) $(LIBMXF_TEST_PATH)/create_test_set.sh primer \
		"./test_primer primer.mxf" "./test_primer primer.mxf_2"

.PHONY: createindextable
createindextable: test_indextable
	@env LIBMXF_TEST_PATH=$(LIBMXF_TEST_PATH) $(LIBMXF_TEST_PATH)/create_test_set.sh indextable \
		"./test_indextable indextable.mxf" "./test_indextable indextable.mxf_2"

.PHONY: createdatamodel
createdatamodel:

.PHONY: createessencecontainer
createessencecontainer: test_essencecontainer
	@env LIBMXF_TEST_PATH=$(LIBMXF_TEST_PATH) $(LIBMXF_TEST_PATH)/create_test_set.sh essencecontainer \
		"./test_essencecontainer essencecontainer.mxf" "./test_essencecontainer essencecontainer.mxf_2"

.PHONY: createheadermetadata
createheadermetadata: test_headermetadata
	@env LIBMXF_TEST_PATH=$(LIBMXF_TEST_PATH) $(LIBMXF_TEST_PATH)/create_test_set.sh headermetadata \
		"./test_headermetadata headermetadata.mxf" "./test_headermetadata headermetadata.mxf_2"

# Turn off no unused parameter warning because some callbacks don't use all the function parameters,
# eg. test_headermetadata.c: before_set_read
CFLAGS += -Wno-unused-parameter

$(LIBMXF_DIR)/libMXF.a:
	$(MAKE) -C $(LIBMXF_DIR)

test_file: $(LIBMXF_DIR)/libMXF.a test_file.o
	$(CC) test_file.o -L$(LIBMXF_DIR) -lMXF $(UUIDLIB) -o test_file

test_file.o: test_file.c $(LIBMXF_DIR)/include/mxf/mxf.h
	$(CC) $(CFLAGS) -c test_file.c

test_partition: $(LIBMXF_DIR)/libMXF.a test_partition.o
	$(CC) test_partition.o -L$(LIBMXF_DIR) -lMXF $(UUIDLIB) -o test_partition

test_partition.o: test_partition.c $(LIBMXF_DIR)/include/mxf/mxf.h
	$(CC) $(CFLAGS) -c test_partition.c

test_primer: $(LIBMXF_DIR)/libMXF.a test_primer.o
	$(CC) test_primer.o -L$(LIBMXF_DIR) -lMXF $(UUIDLIB) -o test_primer

test_primer.o: test_primer.c $(LIBMXF_DIR)/include/mxf/mxf.h
	$(CC) $(CFLAGS) -c test_primer.c

test_indextable: $(LIBMXF_DIR)/libMXF.a test_indextable.o
	$(CC) test_indextable.o -L$(LIBMXF_DIR) -lMXF $(UUIDLIB) -o test_indextable

test_indextable.o: test_indextable.c $(LIBMXF_DIR)/include/mxf/mxf.h
	$(CC) $(CFLAGS) -c test_indextable.c

test_datamodel: $(LIBMXF_DIR)/libMXF.a test_datamodel.o
	$(CC) test_datamodel.o -L$(LIBMXF_DIR) -lMXF $(UUIDLIB) -o test_datamodel

test_datamodel.o: test_datamodel.c $(LIBMXF_DIR)/include/mxf/mxf.h
	$(CC) $(CFLAGS) -c test_datamodel.c

test_essencecontainer: $(LIBMXF_DIR)/libMXF.a test_essencecontainer.o
	$(CC) test_essencecontainer.o -L$(LIBMXF_DIR) -lMXF $(UUIDLIB) -o test_essencecontainer

test_essencecontainer.o: test_essencecontainer.c $(LIBMXF_DIR)/include/mxf/mxf.h
	$(CC) $(CFLAGS) -c test_essencecontainer.c

test_headermetadata: $(LIBMXF_DIR)/libMXF.a test_headermetadata.o
	$(CC) test_headermetadata.o -L$(LIBMXF_DIR) -lMXF $(UUIDLIB) -o test_headermetadata

test_headermetadata.o: test_headermetadata.c $(LIBMXF_DIR)/include/mxf/mxf.h
	$(CC) $(CFLAGS) -c test_headermetadata.c


.PHONY: clean
clean:
	@rm -f *~ *.o 
	@rm -f test_file test_partition test_primer test_indextable test_datamodel test_essencecontainer test_headermetadata
	@rm -f *results_std*.txt
