# Builds WirelessBattery.exe with MinGW-w64 (MSYS2 MINGW64/UCRT64 shell, or any mingw g++ on PATH).
CXX      ?= g++
WINDRES  ?= windres
CXXFLAGS ?= -finput-charset=UTF-8 -fexec-charset=UTF-8 -std=c++17 -Os -Wall -Wno-missing-field-initializers -ffunction-sections -fdata-sections \
            -D_WIN32_WINNT=0x0A00 -DWINVER=0x0A00
LDFLAGS  ?= -municode -mwindows -static -s -Wl,--gc-sections
LIBS     := -lhid -lsetupapi -lcfgmgr32 -lole32 -lcomctl32 -lgdi32 -lruntimeobject

SRC := $(wildcard src/*.cpp) $(wildcard src/drivers/*.cpp)
OBJ := $(patsubst src/%.cpp,build/%.o,$(SRC)) build/app_res.o

WirelessBattery.exe: $(OBJ)
	$(CXX) $(LDFLAGS) $^ -o $@ $(LIBS)

build/%.o: src/%.cpp $(wildcard src/*.h src/drivers/*.h)
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -municode -c $< -o $@

build/app_res.o: res/app.rc res/app.manifest res/app.ico
	@mkdir -p build
	$(WINDRES) -i $< -o $@ --include-dir res

clean:
	rm -rf build WirelessBattery.exe

test:
	@mkdir -p build
	$(CXX) $(CXXFLAGS) tests/airpods_protocol_test.cpp -o build/airpods_protocol_test.exe
	./build/airpods_protocol_test.exe
	$(CXX) $(CXXFLAGS) tests/mchose_driver_test.cpp src/drivers/mchose.cpp -o build/mchose_driver_test.exe
	./build/mchose_driver_test.exe
	$(CXX) -finput-charset=UTF-8 -fexec-charset=UTF-8 -std=c++17 -O2 -Wall tests/rapoo_protocol_test.cpp -o build/rapoo_protocol_test.exe
	./build/rapoo_protocol_test.exe
	$(CXX) $(CXXFLAGS) tests/rapoo_driver_test.cpp src/drivers/rapoo.cpp -o build/rapoo_driver_test.exe
	./build/rapoo_driver_test.exe

.PHONY: clean test
