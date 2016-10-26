a1:test_assign2_1.o buffer_mgr.o buffer_mgr_stat.o dberror.o storage_mgr.o
	gcc test_assign2_1.o buffer_mgr.o buffer_mgr_stat.o dberror.o storage_mgr.o -o output
dberror.o:dberror.c dberror.h
	gcc -c dberror.c
storage_mgr.o:storage_mgr.c storage_mgr.h dberror.h
	gcc -c storage_mgr.c
test_assign2_1.o:test_assign2_1.c test_helper.h dberror.h storage_mgr.h buffer_mgr_stat.h buffer_mgr.h
	gcc -c test_assign2_1.c
buffer_mgr.o:buffer_mgr.c storage_mgr.h buffer_mgr.h
	gcc -c buffer_mgr.c
buffer_mgr_stat.o:buffer_mgr_stat.c buffer_mgr_stat.h buffer_mgr.h
	gcc -c 	buffer_mgr_stat.c
