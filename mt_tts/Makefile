
# RhinoDevel, Marcel Timm, 2025mar14

CXX=g++
CXXFLAGS=-Wall -O2 -std=c++17 -fPIC -DNDEBUG

PIPER_DIR=./piper
PIPER_LIB_DIRS=-L$(PIPER_DIR)/build/pi/lib
PIPER_LIBS=-lespeak-ng -lonnxruntime -lpiper_phonemize
PIPER_INCL=-I$(PIPER_DIR)/build/pi/include -I$(PIPER_DIR)/src/cpp -I$(PIPER_DIR)/build/si/include
LIBRARY=libmttts.so
OBJ=mt_tts.o piper.o

$(LIBRARY): $(OBJ)
	$(CXX) -shared -o $@ $^ $(PIPER_LIB_DIRS) $(PIPER_LIBS)

mt_tts.o: mt_tts.cpp
	$(CXX) $(CXXFLAGS) $(PIPER_INCL) -c $< -o $@

piper.o: $(PIPER_DIR)/src/cpp/piper.cpp
	$(CXX) $(CXXFLAGS) $(PIPER_INCL) -c $< -o $@
	
clean:
	rm -f $(OBJ) $(LIBRARY)

.PHONY: clean
