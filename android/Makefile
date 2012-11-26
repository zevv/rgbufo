
all:
	ant debug

clean:
	ant clean

install: all
	adb install -r bin/rgbufo-debug.apk

run: install
	adb shell am start -n com.zevv.rgbufo/com.zevv.rgbufo.rgbufo
