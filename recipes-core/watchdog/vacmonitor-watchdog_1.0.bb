SUMMARY = "Vacmonitor Watchdog test/repair files"
MAINTAINER = "Gary Oliver <go@robosity.com>"

LICENSE = "GPLv2+"

RDEPENDS_${PN} = "watchdog"

S = "${WORKDIR}"

SRC_URI = "\
	file://vacmonitor-test		\
"
do_install() {
    install -d ${D}${sysconfdir}/watchdog.d/
    install -m 0755 ${S}/vacmonitor-test ${D}${sysconfdir}/watchdog.d/
}

FILES_${PN} += "${sysconfdir}"

