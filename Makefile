
CFLAGS_common := \
	-m64 -march=core2 -ggdb -O3 \
	-DLINUX=1

CXXFLAGS_common := $(CFLAGS_common) \
	-std=c++14 -DBOOST_NO_AUTO_PTR=1 \
	 -I$(BOOST_INCLUDE_DIR)

LDFLAGS_common := -g -m64

CXX := g++
CC  := gcc
LD  := g++
AR  := ar
OBJCOPY := objcopy

CLEAN_DIRS :=

all: work_dir/master/master_server work_dir/download/download_server

include OpenSSL/main.mk
include CryptoPP/main.mk
include soci_lib/main.mk
include MessageServer_lib/main.mk
include protocol/main.mk
include master_server/main.mk
include download_server/main.mk
-include x $(DEPS)

clean:
	$(foreach dir, $(CLEAN_DIRS), rm -r -f $(dir); )
	rm -f work_dir/master/master_server
	rm -f work_dir/download/download_server
