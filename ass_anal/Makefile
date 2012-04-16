CXX=g++
GCC=mingw32-gcc
CFLAGS=-Wall -ggdb -std=c99
OBJS=iolib.o ass_anal.o 
EXE=aa
LIBS=

run: $(EXE)
	$(EXE) ../data/choppers.db ../data/assets.db

$(EXE): $(OBJS) 
	$(GCC) $(CFLAGS) $(OBJS) -o $(EXE)

.cpp.o:
	$(CXX) $(CFLAGS) $< -c -o $@ 

.c.o:
	$(GCC) $(CFLAGS) $< -c -o $@ 

clean:
	@rm -f *.o $(EXE)
