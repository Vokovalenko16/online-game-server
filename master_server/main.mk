PROJ_DIR := master_server

SOURCES := \
	db/Level.cpp \
	db/LevelFileBlock.cpp \
	db/User.cpp \
	db/Token.cpp \
	db/GameDB.cpp \
	db/GameHistoryDB.cpp \
	db/GameRecordDB.cpp \
	db/Member.cpp \
	db/Equipment.cpp \
	db/Equip_list.cpp \
	db/Equip_history.cpp \
	game/Game.cpp \
	game/GameManager.cpp \
	game/PortManager.cpp \
	service/ChildServerAdminServices.cpp \
	service/CommonClientServices.cpp \
	service/GameAdminServices.cpp \
	service/LaunchGameJob.cpp \
	service/LevelServices.cpp \
	service/LevelUploadJob.cpp \
	service/LoadGameServices.cpp \
	service/UserManageServices.cpp \
	service/TokenServices.cpp \
	service/GameService.cpp \
	service/ItemServices.cpp \
	service/ScoreServices.cpp \
	master_server.cpp

	
SOURCES_master_server := $(patsubst %,$(PROJ_DIR)/%,$(SOURCES))
OBJS_master_server := $(patsubst %,$(PROJ_DIR)/release/%.o,$(SOURCES))

LDFLAGS_master_server := $(LDFLAGS_common)
CXXFLAGS_master_server := $(CXXFLAGS_common) -I$(PROJ_DIR) -I$(BOOST_INCLUDE_DIR) \
	-Iprotocol/include -IMessageServer_lib -Isoci_lib -ICryptoPP -IOpenSSL/include
TARGET_master_server := $(PROJ_DIR)/release/master_server.elf

LIBS_master_server := \
	$(TARGET_messageserver_lib) $(TARGET_soci_lib) $(TARGET_cryptopp) $(TARGET_openssl) \
	$(TARGET_protocol) \
	$(BOOST_LIB_X64_DIR)/libboost_serialization.a \
	$(BOOST_LIB_X64_DIR)/libboost_program_options.a \
	$(BOOST_LIB_X64_DIR)/libboost_filesystem.a \
	$(BOOST_LIB_X64_DIR)/libboost_iostreams.a \
	$(BOOST_LIB_X64_DIR)/libboost_system.a \
	$(BOOST_LIB_X64_DIR)/libboost_date_time.a
	

$(PROJ_DIR)/release/%.o: $(PROJ_DIR)/%
	@mkdir -p $(dir $@)
	$(CXX) -c $(CXXFLAGS_master_server) $< -o $@ -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@"

$(TARGET_master_server): $(OBJS_master_server) $(LIBS_master_server)
	@rm -f $@
	$(LD) $(LDFLAGS_master_server) -o $@ $(OBJS_master_server) $(LIBS_master_server) -Xlinker -whole-archive -lpthread -ldl -Xlinker -no-whole-archive -static -static-libgcc

work_dir/master/master_server: $(TARGET_master_server)
	$(OBJCOPY) -S $< $@

DEPS += $(patsubst %.o,%.d,$(OBJS_master_server))

CLEAN_DIRS += $(PROJ_DIR)/release
