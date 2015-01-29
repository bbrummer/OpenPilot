TEMPLATE = subdirs
CONFIG += ordered

SUBDIRS = \
    version_info \
    qscispinbox\
    qtconcurrent \
    aggregation \
    extensionsystem \
    utils \
    opmapcontrol \
    qwt \
    sdlgamepad

OSG_SDK_DIR = $$(OSG_SDK_DIR)
!isEmpty(OSG_SDK_DIR) {
	SUBDIRS += osgearth
}
