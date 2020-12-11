SUMMARY = "Version identifier for vacmonitor software."
HOMEPAGE = "http://www.robosity.com"
MAINTAINER = "Gary Oliver <go@robosity.com>"
SECTION = "vacmonitor"
LICENSE = "GPLv2+"

do_install_append() {
	install -m 755 -d ${D}${sysconfdir}
	echo ${PV} > ${D}${sysconfdir}/${PN}
}

RDEPENDS_${PN} = ""

FILES_${PN} = "${sysconfdir}"

