################################
# Targets to build Marble
#
################################
# Linux prerequisites
################################
#
#
################################
# Windows prerequisites
################################
#
#
################################
# Building
################################
#
# $ make all_marble
#
################################

# TODO should be discovered
QT_VERSION := 5.4.0

MARBLE_NAME_PREFIX :=
MARBLE_NAME_SUFIX := -qt-$(QT_VERSION)

################################
#
# Marble
#
################################

MARBLE_BUILD_CONF := Release
MARBLE_VERSION   := master

MARBLE_GIT_BRANCH := $(MARBLE_VERSION)
MARBLE_BASE_NAME := marble-$(MARBLE_VERSION)

ifeq ($(UNAME), Linux)
	ifeq ($(ARCH), x86_64)
		MARBLE_NAME := $(MARBLE_BASE_NAME)-linux-x64
	else
		MARBLE_NAME := $(MARBLE_BASE_NAME)-linux-x86
	endif
	MARBLE_DATA_BASE_DIR := share/marble/data
	MARBLE_CMAKE_GENERATOR := "Unix Makefiles"
	# for some reason Qt is not added to the path in make/tools.mk
	MARBLE_BUILD_PATH := $(QT_SDK_PREFIX)/bin:$(PATH)
else ifeq ($(UNAME), Darwin)
	MARBLE_NAME := $(MARBLE_BASE_NAME)-clang_64
	MARBLE_DATA_BASE_DIR := share/marble/data
	MARBLE_CMAKE_GENERATOR := "Unix Makefiles"
	# for some reason Qt is not added to the path in make/tools.mk
	MARBLE_BUILD_PATH := $(QT_SDK_PREFIX)/bin:$(PATH)
else ifeq ($(UNAME), Windows)
	MARBLE_NAME := $(MARBLE_BASE_NAME)-$(QT_SDK_ARCH)
	MARBLE_DATA_BASE_DIR := data
	MARBLE_CMAKE_GENERATOR := "MinGW Makefiles"
	# CMake is quite picky about its PATH and will complain if sh.exe is found in it
	MARBLE_BUILD_PATH := "$(TOOLS_DIR)/bin;$(QT_SDK_PREFIX)/bin;$(MINGW_DIR)/bin;$$SYSTEMROOT/System32"
endif

MARBLE_NAME := $(MARBLE_NAME_PREFIX)$(MARBLE_NAME)$(MARBLE_NAME_SUFIX)
MARBLE_SRC_DIR     := $(ROOT_DIR)/3rdparty/marble
MARBLE_BUILD_DIR   := $(BUILD_DIR)/3rdparty/$(MARBLE_NAME)
MARBLE_INSTALL_DIR := $(BUILD_DIR)/3rdparty/install/$(MARBLE_NAME)
MARBLE_DATA_DIR := $(MARBLE_INSTALL_DIR)/$(MARBLE_DATA_BASE_DIR)

.PHONY: marble
marble:
	@$(ECHO) "Building marble $(call toprel, $(MARBLE_SRC_DIR)) into $(call toprel, $(MARBLE_BUILD_DIR))"
	$(V1) $(MKDIR) -p $(MARBLE_BUILD_DIR)
	$(V1) ( $(CD) $(MARBLE_BUILD_DIR) && \
		PATH=$(MARBLE_BUILD_PATH) && \
		$(CMAKE) -G $(MARBLE_CMAKE_GENERATOR) -DCMAKE_BUILD_TYPE=$(MARBLE_BUILD_CONF) \
			-DQTONLY=1 -DQT5BUILD=1 -DWITH_DESIGNER_PLUGIN=0 \
			-DCMAKE_INSTALL_PREFIX=$(MARBLE_INSTALL_DIR) $(MARBLE_SRC_DIR) && \
		$(MAKE) && \
		$(MAKE) install ; \
	)
	@$(ECHO) "Copying restricted maps to $(call toprel, $(MARBLE_DATA_DIR))"
	@$(ECHO) "Copying Google Maps"
	$(V1) $(MKDIR) -p $(MARBLE_DATA_DIR)/maps/earth/googlemaps
	$(V1) $(CP) $(MARBLE_SRC_DIR)/googlemaps/googlemaps.dgml $(MARBLE_DATA_DIR)/maps/earth/googlemaps/
	$(V1) $(CP) $(MARBLE_SRC_DIR)/googlemaps/preview.png $(MARBLE_DATA_DIR)/maps/earth/googlemaps/
	$(V1) $(CP) -R $(MARBLE_SRC_DIR)/googlemaps/0 $(MARBLE_DATA_DIR)/maps/earth/googlemaps/
	@$(ECHO) "Copying Google Sat"
	$(V1) $(MKDIR) -p $(MARBLE_DATA_DIR)/maps/earth/googlesat
	$(V1) $(CP) $(MARBLE_SRC_DIR)/googlesat/googlesat.dgml $(MARBLE_DATA_DIR)/maps/earth/googlesat/
	$(V1) $(CP) $(MARBLE_SRC_DIR)/googlesat/preview.png $(MARBLE_DATA_DIR)/maps/earth/googlesat/
	$(V1) $(CP) -R $(MARBLE_SRC_DIR)/googlesat/0 $(MARBLE_DATA_DIR)/maps/earth/googlesat/
	$(V1) $(CP) -R $(MARBLE_SRC_DIR)/googlesat/bicycle $(MARBLE_DATA_DIR)/maps/earth/googlesat/
	$(V1) $(CP) -R $(MARBLE_SRC_DIR)/googlesat/streets $(MARBLE_DATA_DIR)/maps/earth/googlesat/

.PHONY: package_marble
package_marble:
	@$(ECHO) "Packaging $(call toprel, $(MARBLE_INSTALL_DIR)) into $(notdir $(MARBLE_INSTALL_DIR)).tar"
	$(V1) $(CP) $(ROOT_DIR)/make/3rdparty/OpenPilotReadme.txt $(MARBLE_INSTALL_DIR)/
	$(V1) ( \
		$(CD) $(MARBLE_INSTALL_DIR)/.. && \
		$(TAR) cf $(notdir $(MARBLE_INSTALL_DIR)).tar $(notdir $(MARBLE_INSTALL_DIR)) && \
		$(ZIP) -f $(notdir $(MARBLE_INSTALL_DIR)).tar && \
		$(call MD5_GEN_TEMPLATE,$(notdir $(MARBLE_INSTALL_DIR)).tar.gz) ; \
	)

.NOTPARALLEL:
.PHONY: prepare_marble
prepare_marble: clone_marble

.PHONY: clone_marble
clone_marble:
	$(V1) if [ -d "$(MARBLE_SRC_DIR)" ]; then \
		$(ECHO) "Cloning restricted maps to $(call toprel, $(MARBLE_SRC_DIR))" ; \
		$(GIT) clone https://gitorious.org/marble-restricted-maps/googlemaps.git $(MARBLE_SRC_DIR)/googlemaps ; \
		$(GIT) clone https://gitorious.org/marble-restricted-maps/googlesat.git $(MARBLE_SRC_DIR)/googlesat ; \
	else \
		$(MKDIR) -p $(MARBLE_SRC_DIR) ; \
		$(ECHO) "Cloning marble to $(call toprel, $(MARBLE_SRC_DIR))" ; \
		$(GIT) clone -b $(MARBLE_GIT_BRANCH) git://anongit.kde.org/marble $(MARBLE_SRC_DIR) ; \
		$(ECHO) "Cloning restricted maps to $(call toprel, $(MARBLE_SRC_DIR))" ; \
		$(GIT) clone https://gitorious.org/marble-restricted-maps/googlemaps.git $(MARBLE_SRC_DIR)/googlemaps ; \
		$(GIT) clone https://gitorious.org/marble-restricted-maps/googlesat.git $(MARBLE_SRC_DIR)/googlesat ; \
	fi

.PHONY: clean_marble
clean_marble:
	@$(ECHO) $(MSG_CLEANING) $(call toprel, $(MARBLE_BUILD_DIR))
	$(V1) [ ! -d "$(MARBLE_BUILD_DIR)" ] || $(RM) -r "$(MARBLE_BUILD_DIR)"
	@$(ECHO) $(MSG_CLEANING) $(call toprel, $(MARBLE_INSTALL_DIR))
	$(V1) [ ! -d "$(MARBLE_INSTALL_DIR)" ] || $(RM) -r "$(MARBLE_INSTALL_DIR)"

.PHONY: clean_all_marble
clean_all_marble: clean_marble
	@$(ECHO) $(MSG_CLEANING) $(call toprel, $(MARBLE_SRC_DIR))
	$(V1) [ ! -d "$(MARBLE_SRC_DIR)" ] || $(RM) -r "$(MARBLE_SRC_DIR)"

.NOTPARALLEL:
.PHONY: all_marble
all_marble: prepare_marble marble package_marble