PROJ_DIR := CryptoPP

SOURCES := \
src/3way.cpp \
src/adler32.cpp \
src/algebra.cpp \
src/algparam.cpp \
src/arc4.cpp \
src/asn.cpp \
src/authenc.cpp \
src/base32.cpp \
src/base64.cpp \
src/basecode.cpp \
src/bfinit.cpp \
src/blowfish.cpp \
src/blumshub.cpp \
src/camellia.cpp \
src/cast.cpp \
src/casts.cpp \
src/cbcmac.cpp \
src/ccm.cpp \
src/channels.cpp \
src/cmac.cpp \
src/cpu.cpp \
src/crc.cpp \
src/cryptlib.cpp \
src/cryptlib_bds.cpp \
src/default.cpp \
src/des.cpp \
src/dessp.cpp \
src/dh.cpp \
src/dh2.cpp \
src/dll.cpp \
src/dsa.cpp \
src/eax.cpp \
src/ec2n.cpp \
src/eccrypto.cpp \
src/ecp.cpp \
src/elgamal.cpp \
src/emsa2.cpp \
src/eprecomp.cpp \
src/esign.cpp \
src/files.cpp \
src/filters.cpp \
src/fips140.cpp \
src/gcm.cpp \
src/gf256.cpp \
src/gf2_32.cpp \
src/gf2n.cpp \
src/gfpcrypt.cpp \
src/gost.cpp \
src/gzip.cpp \
src/hex.cpp \
src/hmac.cpp \
src/hrtimer.cpp \
src/ida.cpp \
src/idea.cpp \
src/integer.cpp \
src/iterhash.cpp \
src/luc.cpp \
src/mars.cpp \
src/marss.cpp \
src/md2.cpp \
src/md4.cpp \
src/md5.cpp \
src/misc.cpp \
src/modes.cpp \
src/mqueue.cpp \
src/mqv.cpp \
src/nbtheory.cpp \
src/network.cpp \
src/oaep.cpp \
src/osrng.cpp \
src/panama.cpp \
src/pch.cpp \
src/pkcspad.cpp \
src/polynomi.cpp \
src/pssr.cpp \
src/pubkey.cpp \
src/queue.cpp \
src/rabin.cpp \
src/randpool.cpp \
src/rc2.cpp \
src/rc5.cpp \
src/rc6.cpp \
src/rdtables.cpp \
src/rijndael.cpp \
src/ripemd.cpp \
src/rng.cpp \
src/rsa.cpp \
src/rw.cpp \
src/safer.cpp \
src/salsa.cpp \
src/seal.cpp \
src/seed.cpp \
src/serpent.cpp \
src/sha.cpp \
src/sha3.cpp \
src/shacal2.cpp \
src/shark.cpp \
src/sharkbox.cpp \
src/simple.cpp \
src/skipjack.cpp \
src/sosemanuk.cpp \
src/square.cpp \
src/squaretb.cpp \
src/strciphr.cpp \
src/tea.cpp \
src/tftables.cpp \
src/tiger.cpp \
src/tigertab.cpp \
src/trdlocal.cpp \
src/ttmac.cpp \
src/twofish.cpp \
src/vmac.cpp \
src/wait.cpp \
src/wake.cpp \
src/whrlpool.cpp \
src/winpipes.cpp \
src/xtr.cpp \
src/xtrcrypt.cpp \
src/zdeflate.cpp \
src/zinflate.cpp \
src/zlib.cpp

	
SOURCES_cryptopp := $(patsubst %,$(PROJ_DIR)/%,$(SOURCES))
OBJS_cryptopp := $(patsubst %,$(PROJ_DIR)/release/%.o,$(SOURCES))

CXXFLAGS_cryptopp := $(CXXFLAGS_common) -DCRYPTOPP_DISABLE_AESNI -I$(PROJ_DIR)/CryptoPP -I$(PROJ_DIR)/src
TARGET_cryptopp := $(PROJ_DIR)/release/cryptopp.a

$(PROJ_DIR)/release/%.o: $(PROJ_DIR)/%
	@mkdir -p $(dir $@)
	$(CXX) -c $(CXXFLAGS_cryptopp) $< -o $@ -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@"

$(TARGET_cryptopp): $(OBJS_cryptopp)
	@rm -f $@
	$(AR) -r $@ $(OBJS_cryptopp)

DEPS += $(patsubst %.o,%.d,$(OBJS_cryptopp))

CLEAN_DIRS += $(PROJ_DIR)/release
