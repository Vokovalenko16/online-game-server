PROJ_DIR := protocol

SOURCES := \
	src/base16.cpp

SOURCES_protocol := $(patsubst %,$(PROJ_DIR)/%,$(SOURCES))
OBJS_protocol := $(patsubst %,$(PROJ_DIR)/release/%.o,$(SOURCES))

LDFLAGS_protocol := $(LDFLAGS_common)
CFLAGS_protocol := $(CFLAGS_common) -I$(PROJ_DIR)
CXXFLAGS_protocol := $(CXXFLAGS_common) -I$(PROJ_DIR) -I$(BOOST_INCLUDE_DIR)
TARGET_protocol := $(PROJ_DIR)/release/protocol.a

$(PROJ_DIR)/release/%.cpp.o: $(PROJ_DIR)/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) -c $(CXXFLAGS_protocol) $< -o $@ -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@"

$(PROJ_DIR)/release/%.c.o: $(PROJ_DIR)/%.c
	@mkdir -p $(dir $@)
	$(CC) -c $(CFLAGS_protocol) $< -o $@ -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@"

$(TARGET_protocol): $(OBJS_protocol)
	@rm -f $@
	$(AR) -r $@ $(OBJS_protocol)

DEPS += $(patsubst %.o,%.d,$(OBJS_protocol))

CLEAN_DIRS += $(PROJ_DIR)/release
