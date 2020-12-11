SUMMARY = "Systemd package for rootresizer"
HOMEPAGE = "http://www.cpac.com"
MAINTAINER = "Gary Oliver <go@ao-cs.com>"
SECTION = "cpac"
LICENSE = "CLOSED"
PR = "r0"

inherit systemd

DEPENDS = " \
"

RDEPENDS_${PN} = " \
	bash				\
	coreutils			\
	rootresizer			\
"

S = "${WORKDIR}"

SRC_URI = " \
	file://rootresizer.service	\
"

do_install() {
	install -d ${D}${systemd_unitdir}/system/
	install -m 0644 ${WORKDIR}/rootresizer.service ${D}${systemd_unitdir}/system/
}

SYSTEMD_SERVICE_${PN} = "rootresizer.service"

FILES_${PN} += "${systemd_unitdir}/system/*"

