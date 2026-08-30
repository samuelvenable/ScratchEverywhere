#!/bin/sh
cd "${0%/*}";
if [ -d "build" ]; then
	rm -fr build;
fi;
if [ ! -d "build" ]; then
	mkdir build;
fi;
if [ -d "build" ]; then
	cd build;
	if [ "$OS" = "Windows_NT" ]; then
		cmake .. -DCMAKE_BUILD_TYPE=Release -DSE_RENDERER="sdl3" -DSE_WINDOWING="sdl3" -DSE_AUDIO_ENGINE="sdl3" -DSE_CLOUDVARS=OFF -DSE_DOWNLOAD=OFF -DSE_USE_LIBRARY_BUILD=OFF && ninja;
		cmake .. -DCMAKE_BUILD_TYPE=Release -DSE_RENDERER="sdl3" -DSE_WINDOWING="sdl3" -DSE_AUDIO_ENGINE="sdl3" -DSE_CLOUDVARS=OFF -DSE_DOWNLOAD=OFF -DSE_USE_LIBRARY_BUILD=ON && ninja;
    else
		cmake .. -DCMAKE_BUILD_TYPE=Release -DSE_RENDERER="sdl3" -DSE_WINDOWING="sdl3" -DSE_AUDIO_ENGINE="sdl3" -DSE_CLOUDVARS=OFF -DSE_DOWNLOAD=OFF -DSE_USE_LIBRARY_BUILD=OFF && make;
		cmake .. -DCMAKE_BUILD_TYPE=Release -DSE_RENDERER="sdl3" -DSE_WINDOWING="sdl3" -DSE_AUDIO_ENGINE="sdl3" -DSE_CLOUDVARS=OFF -DSE_DOWNLOAD=OFF -DSE_USE_LIBRARY_BUILD=ON && make;
	fi;
fi;
