#!/bin/sh

gcc -O2 -w -m32 -shared -I./hlsdk/common -I./hlsdk/dlls -I./hlsdk/engine -I./hlsdk/pm_shared -I./metamod dllapi.cpp engine_api.cpp h_export.cpp meta_api.cpp sdk_util.cpp -o precache_mm.so
