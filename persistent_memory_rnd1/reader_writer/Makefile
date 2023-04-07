rw_ll.o : read_write_linked_list.cc
	g++ read_write_linked_list.cc -lpmemobj -o rw_ll.o

read_write.o : read_write.c
	gcc read_write.c -lpmemobj -o read_write.o

clean :
	rm *.o