all: libklargest.so test

libklargest.so: klargest.cpp
	g++ -g -shared -fPIC -o $@ $^

test: main.cpp
	g++ -g -L. -o $@ $^ -lklargest

clean:
	rm -f *.o *.so test
