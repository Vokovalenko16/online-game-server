PROJ_DIR := soci_lib

SOURCES := \
	soci/backends/sqlite/soci_sqlite3_blob.cpp \
	soci/backends/sqlite/soci_sqlite3_common.cpp \
	soci/backends/sqlite/soci_sqlite3_factory.cpp \
	soci/backends/sqlite/soci_sqlite3_row-id.cpp \
	soci/backends/sqlite/soci_sqlite3_session.cpp \
	soci/backends/sqlite/soci_sqlite3_standard-into-type.cpp \
	soci/backends/sqlite/soci_sqlite3_standard-use-type.cpp \
	soci/backends/sqlite/soci_sqlite3_statement.cpp \
	soci/backends/sqlite/soci_sqlite3_vector-into-type.cpp \
	soci/backends/sqlite/soci_sqlite3_vector-use-type.cpp \
	soci/blob.cpp \
	soci/connection-pool.cpp \
	soci/error.cpp \
	soci/into-type.cpp \
	soci/once-temp-type.cpp \
	soci/prepare-temp-type.cpp \
	soci/procedure.cpp \
	soci/ref-counted-prepare-info.cpp \
	soci/ref-counted-statement.cpp \
	soci/row.cpp \
	soci/rowid.cpp \
	soci/session.cpp \
	soci/soci-simple.cpp \
	soci/statement.cpp \
	soci/transaction.cpp \
	soci/use-type.cpp \
	soci/values.cpp \
	sqlite/shell.c \
	sqlite/sqlite3.c

SOURCES_soci_lib := $(patsubst %,$(PROJ_DIR)/%,$(SOURCES))
OBJS_soci_lib := $(patsubst %,$(PROJ_DIR)/release/%.o,$(SOURCES))

LDFLAGS_soci_lib := $(LDFLAGS_common)
CFLAGS_soci_lib := $(CFLAGS_common) -I$(PROJ_DIR)
CXXFLAGS_soci_lib := $(CXXFLAGS_common) -I$(PROJ_DIR) -I$(BOOST_INCLUDE_DIR)
TARGET_soci_lib := $(PROJ_DIR)/release/soci_lib.a

$(PROJ_DIR)/release/%.cpp.o: $(PROJ_DIR)/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) -c $(CXXFLAGS_soci_lib) $< -o $@ -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@"

$(PROJ_DIR)/release/%.c.o: $(PROJ_DIR)/%.c
	@mkdir -p $(dir $@)
	$(CC) -c $(CFLAGS_soci_lib) $< -o $@ -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@"

$(TARGET_soci_lib): $(OBJS_soci_lib)
	@rm -f $@
	$(AR) -r $@ $(OBJS_soci_lib)

DEPS += $(patsubst %.o,%.d,$(OBJS_soci_lib))

CLEAN_DIRS += $(PROJ_DIR)/release
