SUMMARY = "Updater for installed signed packages"
HOMEPAGE = "http://www.robosity.com"
MAINTAINER = "Gary Oliver <go@robosity.com>"
SECTION = "vacmonitor"
LICENSE = "GPLv2"
LIC_FILES_CHKSUM = "file://generic_GPLv2.txt;md5=801f80980d171dd6425610833a22dbe6"
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
	file://${BPN}				\
	file://generic_GPLv2.txt		\
	file://wait-for-vacsystemupdater	\
"

do_install() {
	install -m 0755 -d ${D}${sbindir}
	install -m 0755 ${PN} ${D}${sbindir}
	install -m 0755 wait-for-vacsystemupdater ${D}${sbindir}
}


FILES_${PN} += "${sbindir}"

