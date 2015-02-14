#
# Linux-specific packaging script
#

ifndef OPENPILOT_IS_COOL
    $(error Top level Makefile must be used to build this target)
endif

DEB_VER			:= $(subst RELEASE-,,$(PACKAGE_LBL))-1
DEB_DIR			:= $(ROOT_DIR)/package/linux/debian
DEB_BUILD_DIR		:= $(ROOT_DIR)/debian

SED_DATE_STRG		= $(shell date -R)
SED_SCRIPT		= s/<VERSION>/$(DEB_VER)/;s/<DATE>/$(SED_DATE_STRG)/

DEB_ARCH		:= $(shell dpkg --print-architecture)
DEB_PACKAGE_NAME	:= openpilot_$(DEB_VER)_$(DEB_ARCH)

.PHONY: package
package:
	$(V1) echo "Building Linux package, please wait..."
	$(V1) cp -rL $(DEB_DIR) $(DEB_BUILD_DIR)
	$(V1) sed -i -e "$(SED_SCRIPT)" $(DEB_BUILD_DIR)/changelog
	$(V1) dpkg-buildpackage -b -us -uc
	$(V1) mv $(ROOT_DIR)/../$(DEB_PACKAGE_NAME).deb $(BUILD_DIR)/$(DEB_PACKAGE_NAME).deb
	$(V1) mv $(ROOT_DIR)/../$(DEB_PACKAGE_NAME).changes $(BUILD_DIR)/$(DEB_PACKAGE_NAME).changes
	$(V1) rm -rf $(DEB_BUILD_DIR)

##############################
#
# Install OpenPilot
#
##############################
prefix  := /usr/local
bindir  := $(prefix)/bin
libdir  := $(prefix)/lib
datadir := $(prefix)/share

INSTALL = cp -a --no-preserve=ownership
LN = ln
LN_S = ln -s

.PHONY: install
install:
	@$(ECHO) " INSTALLING GCS TO $(DESTDIR)/)"
	$(V1) $(MKDIR) -p $(DESTDIR)$(bindir)
	$(V1) $(MKDIR) -p $(DESTDIR)$(libdir)
	$(V1) $(MKDIR) -p $(DESTDIR)$(datadir)
	$(V1) $(MKDIR) -p $(DESTDIR)$(datadir)/applications
	$(V1) $(MKDIR) -p $(DESTDIR)$(datadir)/pixmaps
	$(V1) $(MKDIR) -p $(DESTDIR)$(udevdir)
	$(V1) $(INSTALL) $(BUILD_DIR)/openpilotgcs_$(GCS_BUILD_CONF)/bin/openpilotgcs $(DESTDIR)$(bindir)
	$(V1) $(INSTALL) $(BUILD_DIR)/openpilotgcs_$(GCS_BUILD_CONF)/bin/udp_test $(DESTDIR)$(bindir)
	$(V1) $(INSTALL) $(BUILD_DIR)/openpilotgcs_$(GCS_BUILD_CONF)/lib/openpilotgcs $(DESTDIR)$(libdir)
	$(V1) $(INSTALL) $(BUILD_DIR)/openpilotgcs_$(GCS_BUILD_CONF)/share/openpilotgcs $(DESTDIR)$(datadir)
	$(V1) $(INSTALL) $(ROOT_DIR)/package/linux/openpilot.desktop $(DESTDIR)$(datadir)/applications
	$(V1) $(INSTALL) $(ROOT_DIR)/package/linux/openpilot.png $(DESTDIR)$(datadir)/pixmaps
	$(V1) rm $(DESTDIR)/$(datadir)/openpilotgcs/translations/Makefile


