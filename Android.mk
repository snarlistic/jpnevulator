LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := jpnevulator

# we need at least API level 21 for posix_openpt
TARGET_PLATFORM := android-21

LOCAL_CFLAGS += -Wall

LOCAL_SRC_FILES := main.c \
	options.c \
	jpnevulator.c \
	byte.c \
	interface.c \
	tty.c \
	pty.c \
	io.c \
	checksum.c \
	crc16.c \
	crc8.c \
	list.c \
	misc.c

include $(BUILD_EXECUTABLE)
