default: mylisp

mylisp: mylisp.c
	gcc $< mpc/mpc.c -o $@ -lreadline -Wall -pedantic -Wextra -Werror

run: mylisp
	./mylisp

clean:
	@rm -f mylisp *.o

