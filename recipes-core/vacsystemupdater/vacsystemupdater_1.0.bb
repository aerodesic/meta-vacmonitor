#
# Recipe for system updater program
#
SRCREV = "${AUTOREV}"

require vacsystemupdater-${PV}.inc

RDEPENDS_${PN} += " \
	binutils		\
	parted			\
	e2fsprogs-resize2fs	\
	parted			\
	util-linux		\
"

