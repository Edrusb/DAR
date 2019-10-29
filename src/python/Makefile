## Gnu make specific:
PYEXT=$(shell python3-config --extension-suffix)
PYFLAGS=$(shell python3 -m pybind11 --includes)

CXXFLAGS=-std=c++11 -fPIC $(PYFLAGS)
LDFLAGS=-shared -ldar64 -lpthread -lrsync -lattr -lgcrypt -lgpg-error -llzma -llzo2 -lbz2 -lz -ldl -lcap  -L/usr/lib/x86_64-linux-gnu -lgpgme -lassuan -lgpg-error -L/usr/local/lib -lcurl -L/usr/local/lib -lthreadar -lpthread


.cpp.o:
	$(CXX) -c $(CXXFLAGS) $< -o $@

libdar$(PYEXT): pybind11_libdar.o
	$(CXX) $(LDFLAGS) $(PYFLAGS) $<  -o $@

clean:
	rm -f *.o
	rm -f libdar$(PYEXT)
