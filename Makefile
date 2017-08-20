default: mylisp

mylisp: mylisp.c
	gcc $< -o $@ -lreadline

run: mylisp
	./mylisp

clean:
	@rm -f mylisp *.o

