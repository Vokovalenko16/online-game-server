PROJ_DIR := MessageServer_lib

SOURCES := \
	core/Exception.cpp \
	db/Database.cpp \
	http/Authorizer.cpp \
	http/HttpHandler.cpp \
	http/HttpServer.cpp \
	http/StaticContentHandler.cpp \
	job/JobManager.cpp \
	json/src/json_reader.cpp \
	json/src/json_value.cpp \
	json/src/json_writer.cpp \
	logger/Logger.cpp \
	net/MessageServer.cpp \
	net/TlsConnection.cpp \
	net/TlsContext.cpp \
	net/TlsHandshaker.cpp \
	net/TlsListener.cpp \
	util/FileUtil.cpp \
	util/cpuinfo.cpp \
	util/io_worker.cpp \
	http/GameCenterConnection.cpp \
	net/Future.cpp
	
SOURCES_messageserver_lib := $(patsubst %,$(PROJ_DIR)/%,$(SOURCES))
OBJS_messageserver_lib := $(patsubst %,$(PROJ_DIR)/release/%.o,$(SOURCES))

LDFLAGS_messageserver_lib := $(LDFLAGS_common)
CXXFLAGS_messageserver_lib := $(CXXFLAGS_common) -I$(PROJ_DIR) -I$(BOOST_INCLUDE_DIR) \
	-Isoci_lib -Iprotocol/include -IOpenSSL/include -ICryptoPP \
	-DLINUX=1
TARGET_messageserver_lib := $(PROJ_DIR)/release/messageserver_lib.a

$(PROJ_DIR)/release/%.o: $(PROJ_DIR)/%
	@mkdir -p $(dir $@)
	$(CXX) -c $(CXXFLAGS_messageserver_lib) $< -o $@ -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@"

$(TARGET_messageserver_lib): $(OBJS_messageserver_lib)
	@rm -f $@
	$(AR) -r $@ $(OBJS_messageserver_lib)

DEPS += $(patsubst %.o,%.d,$(OBJS_messageserver_lib))

CLEAN_DIRS += $(PROJ_DIR)/release
