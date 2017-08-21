default: mylisp

mylisp: mylisp.c
	gcc $< mpc/mpc.c -o $@ -lreadline

run: mylisp
	./mylisp

clean:
	@rm -f mylisp *.o

