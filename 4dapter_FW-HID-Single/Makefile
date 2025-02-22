# To generate the fqbn (Fully Qualified Board Name) for the "BOARD" variable
# below, if you are already using the IDE look at preferences.txt (Linked from
# file/Preferences). Find:
# - target_package (example: SparkFun)
# - target_platform (should be: avr)
# - board (example: promicro)
# - cpu (should be 16MHzatmega32U4)
#
# For instance with a SparkFun Pro Micro the fqbn becomes the |BOARD| variable below.
# To install the needed dependencies look at the install-sparkfun example below and
# modify to your needs.

BOARD = SparkFun:avr:promicro:cpu=16MHzatmega32U4

SERIAL = /dev/ttyACM0

SPARKFUN-URLS = https://raw.githubusercontent.com/sparkfun/Arduino_Boards/master/IDE_Board_Manager/package_sparkfun_index.json

EXTRA_FLAGS =
OUTPUT_DIR = build

# This is believed to match how arduino-cli works, i.e. it uses the name of the
# current directory to infer the name of the main .ino file.
PROJECT_FILE = $(notdir $(CURDIR)).ino


DEBUG ?= 0
ifeq ($(DEBUG), 1)
  EXTRA_FLAGS += --build-property compiler.cpp.extra_flags=-DDEBUG
  OUTPUT_DIR = debug-build
endif

all: compile

clean:
	rm -rf $(OUTPUT_DIR)

compile: $(OUTPUT_DIR)/$(PROJECT_FILE).hex

$(OUTPUT_DIR)/$(PROJECT_FILE).hex: $(wildcard *.cpp) $(wildcard *.h) $(PROJECT_FILE)
	arduino-cli compile $(EXTRA_FLAGS) --output-dir "$(OUTPUT_DIR)" -b "$(BOARD)" -e

upload: compile
	arduino-cli upload -v --input-dir "$(OUTPUT_DIR)" -b  "$(BOARD)" -p $(SERIAL) -t

install-sparkfun:
	arduino-cli core update-index --additional-urls "$(SPARKFUN-URLS)"
	arduino-cli core install arduino:avr Sparkfun:avr --additional-urls "$(SPARKFUN-URLS)"
