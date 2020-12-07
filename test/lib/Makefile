libtest.so: wrap.c
	gcc -g -o $@ -shared -fPIC $^ -lprobe

clean:
	rm -f *.so *.o
