PROJ_DIR := download_server

SOURCES := \
	db/File_Blocks_Model.cpp \
	db/Files_Model.cpp \
	db/Releases_Model.cpp \
	service/AdminClientServices.cpp \
	service/CommonClientServices.cpp \
	service/DirectoryLoadJob.cpp \
	download_server.cpp
	
SOURCES_download_server := $(patsubst %,$(PROJ_DIR)/%,$(SOURCES))
OBJS_download_server := $(patsubst %,$(PROJ_DIR)/release/%.o,$(SOURCES))

LDFLAGS_download_server := $(LDFLAGS_common)
CXXFLAGS_download_server := $(CXXFLAGS_common) -I$(PROJ_DIR) -I$(BOOST_INCLUDE_DIR) \
	-Iprotocol/include -IMessageServer_lib -Isoci_lib -ICryptoPP
TARGET_download_server := $(PROJ_DIR)/release/download_server.elf

LIBS_download_server := \
	$(TARGET_messageserver_lib) $(TARGET_soci_lib) $(TARGET_cryptopp) $(TARGET_openssl) \
	$(TARGET_protocol) \
	$(BOOST_LIB_X64_DIR)/libboost_serialization.a \
	$(BOOST_LIB_X64_DIR)/libboost_program_options.a \
	$(BOOST_LIB_X64_DIR)/libboost_filesystem.a \
	$(BOOST_LIB_X64_DIR)/libboost_iostreams.a \
	$(BOOST_LIB_X64_DIR)/libboost_system.a \
	$(BOOST_LIB_X64_DIR)/libboost_date_time.a \

$(PROJ_DIR)/release/%.o: $(PROJ_DIR)/%
	@mkdir -p $(dir $@)
	$(CXX) -c $(CXXFLAGS_download_server) $< -o $@ -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@"

$(TARGET_download_server): $(OBJS_download_server) $(LIBS_download_server)
	@rm -f $@
	$(LD) $(LDFLAGS_download_server) -o $@ $(OBJS_download_server) $(LIBS_download_server) -Xlinker -whole-archive -lpthread -ldl -Xlinker -no-whole-archive -static -static-libgcc

work_dir/download/download_server: $(TARGET_download_server)
	$(OBJCOPY) -S $< $@

DEPS += $(patsubst %.o,%.d,$(OBJS_download_server))

CLEAN_DIRS += $(PROJ_DIR)/release
