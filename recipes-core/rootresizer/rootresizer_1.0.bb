#
# Recipe for system updater program
#
SRCREV = "${AUTOREV}"

require rootresizer.inc

RDEPENDS_${PN} += " \
	parted			\
	e2fsprogs-resize2fs	\
	parted			\
	util-linux		\
"
