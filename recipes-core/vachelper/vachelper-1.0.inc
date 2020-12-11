SUMMARY = "Updater for installed signed packages"
HOMEPAGE = "http://www.cpac.com"
MAINTAINER = "Gary Oliver <go@ao-cs.com>"
SECTION = "cpac"
LICENSE = "CLOSED"
PR = "r1"

inherit systemd

DEPENDS = " \
"

RDEPENDS_${PN} = " \
	bash		\
	coreutils	\
"

#	gnupg

S = "${WORKDIR}"

SRC_URI = " \
	file://vachelper.c	\
"

do_compile() {
	${CC} ${LDFLAGS} vachelper.c -o vachelper
}

do_install() {
        install -m 0755 -d ${D}${sbindir}
	install -m 6755 vachelper ${D}${sbindir}
}


FILES_${PN} = "${bindir} ${sbindir} ${sysconfdir} ${systemd_unitdir}"

