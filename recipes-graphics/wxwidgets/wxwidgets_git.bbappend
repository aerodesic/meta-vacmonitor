FILESEXTRAPATHS_prepend := "${THISDIR}/${PN}:"

SRC_URI += " \
	file://0001-Fix-cmake-install-path-for-wx-config.patch	\
"

# Cross install under Yocto.  CMAKE_INSTALL_PREFIX_YOCTO is referenced in above patch.
EXTRA_OECMAKE += " \
	-DCMAKE_INSTALL_LOCAL_ONLY=yocto	\
	-DDESTDIR=${D}				\
"

FILES_${PN}-bin = ""
