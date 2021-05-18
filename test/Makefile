all: libklargest.so test

libklargest.so: klargest.cpp
	g++ -g -shared -fPIC -o $@ $^

test: main.cpp
	g++ -g -Wl,-rpath=/home/jiaru/lprofile/test/ -L. -o $@ $^ -lklargest -lpthread

clean:
	rm -f *.o *.so test
