CPP = g++
LOAD_GENERATOR_FILE = load_gen.cpp

all: load_generator

load_generator: $(LOAD_GENERATOR_FILE)
	$(CPP) $(LOAD_GENERATOR_FILE) -pthread -o load_generator

clean:
	rm -f load_generator