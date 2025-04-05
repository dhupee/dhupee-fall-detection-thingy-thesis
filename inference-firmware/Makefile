# when running using ENV value, do something like:
# `make upload-env ENV=esp32doit-devkit-v1-80`

all:
	platformio -f  run

upload:
	platformio -f  run --target upload

clean:
	platformio -f  run --target clean

monitor:
	platformio device monitor

program:
	platformio -f  run --target program

uploadfs:
	platformio -f  run --target uploadfs

upload-env:
	platformio -f  run --target upload -e $(ENV)

clean-env:
	platformio -f  run --target clean -e $(ENV)

program-env:
	platformio -f  run --target program -e $(ENV)

uploadfs-env:
	platformio -f  run --target uploadfs -e $(ENV)

update:
	platformio -f  update

