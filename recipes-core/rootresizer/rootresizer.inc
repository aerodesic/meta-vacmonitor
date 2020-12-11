SUMMARY = "Root resizer"
HOMEPAGE = "http://www.cpac.com"
MAINTAINER = "Gary Oliver <go@ao-cs.com>"
SECTION = "cpac"
LICENSE = "CLOSED"
PR = "r0"

DEPENDS = " \
"

RDEPENDS_${PN} = " \
	bash				\
	coreutils			\
	gnupg				\
	python3				\
"

S = "${WORKDIR}"

SRC_URI = " \
	file://${PN}			\
"

do_install() {
	install -m 0755 -d ${D}${sbindir}
	install -m 0755 ${PN} ${D}${sbindir}
}


FILES_${PN} += "${sbindir}"

