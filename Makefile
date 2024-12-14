# pivec application Makefile

# Makefile Targets
#          all:	compiles the source code
#        clean: removes all .hex, .elf, and .o files in the source code and 
#              	library directories
#      install:	installs the application and documentation
#    uninstall:	uninstalls the application and documentation
#   permission:	add user to kmem group and setup a udev	rule to make 
# 				/dev/mem read/writeable by kmem user/group
# unpermission:	Remove the udev rule
#         boot:	Create a .service file to launch application using systemd.
#				This file will use the OPTIONS defined in the makefile or the
#				make command line
#       unboot:	Remove the .service file from the systemd configuration 
#				directory

# Build Variables +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
# Project name
TARGET = pivec
# Linux usr binaries directory
BINDIR =	/usr/local/bin
# Linux manual directory for the man command
MANDIR =	/usr/local/man/man1
# Build directory
BUILD_DIR = ./build
# Systemd services directory
SYSDDIR = /etc/systemd/system
# Udev rules directory
UDEVDIR = /etc/udev/rules.d
# C compiler command
CC =		gcc
# Build flags for c files
CFLAGS =	-O -I/usr/local/include -pedantic -Wall -Wpointer-arith -Wshadow -Wcast-qual -Wcast-align -Wstrict-prototypes -Wmissing-prototypes -Wmissing-declarations -Wredundant-decls -Wno-long-long
# Linker Flags
LFLAGS =	-lbcm_host
# Application command line options
OPTIONS = -c off

# Build Targets +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
# Build all target files
all:		build $(TARGET)
# Create the build directory
build:
	mkdir -p $(BUILD_DIR)
# Build the app from the .c source
$(TARGET):		$(TARGET).c
	$(CC) $(CFLAGS) $(TARGET).c -o $(BUILD_DIR)/$(TARGET) $(LFLAGS)
# Run serkey with the provided args
run:		all
	$(BUILD_DIR)/$(TARGET) $(OPTIONS)
# Install the application
install:	all
	sudo cp $(BUILD_DIR)/$(TARGET) $(BINDIR)
	sudo cp $(TARGET).1 $(MANDIR)
# Uninstall the application
uninstall:
	sudo rm -f $(BINDIR)/$(TARGET)
	sudo rm -f $(MANDIR)/$(TARGET).1
# Setup permissions
permission:
	-sudo usermod -a -G kmem $$(whoami)
	-sudo cp ./mem.rules $(UDEVDIR)
	sudo udevadm control --reload-rules
# Remove the rule that puts /dev/uinput in the uinput group
unpermission:
	-sudo deluser $$(whoami) kmem
	sudo rm -f $(UDEVDIR)/mem.rules
	sudo udevadm control --reload-rules
# Setup daemon launched by systemd
boot: install
	cp $(TARGET).service.src $(TARGET).service
	echo ExecStart=$(BINDIR)/serkey $(OPTIONS) | tee -a $(TARGET).service
	sudo mv $(TARGET).service $(SYSDDIR)
	systemctl start $(TARGET)
	systemctl enable $(TARGET)
	systemctl status $(TARGET)
# Remove the daemon from systemd
unboot:
	systemctl stop $(TARGET)
	systemctl disable $(TARGET)
	sudo rm -f $(SYSDDIR)/$(TARGET).service
# Install prereqeuisites
prereqs:
	sudo apt update
	sudo apt install libraspberrypi-dev raspberrypi-kernel-headers
# Clean up all generated files
clean:
	rm -rf $(BUILD_DIR)
