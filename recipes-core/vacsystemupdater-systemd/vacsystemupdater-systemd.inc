SUMMARY = "Updater for installed signed packages"
HOMEPAGE = "http://www.robosity.com"
MAINTAINER = "Gary Oliver <go@robosity.com>"
SECTION = "vacmonitor"
LICENSE = "GPLv2"
LIC_FILES_CHKSUM = "file://generic_GPLv2.txt;md5=801f80980d171dd6425610833a22dbe6"
PR = "r0"

inherit systemd

DEPENDS = " \
"

RDEPENDS_${PN} = " \
	bash				\
	coreutils			\
	vacsystemupdater		\
"

S = "${WORKDIR}"

SRC_URI = " \
	file://generic_GPLv2.txt	\
	file://vacsystemupdater.service	\
"

do_install() {
	#### install -d ${D}${systemd_unitdir}/system/
	#### install -m 0644 ${WORKDIR}/vacsystemupdater.service ${D}${systemd_unitdir}/system/

	install -d ${D}${systemd_system_unitdir}/
	install -m 0644 ${WORKDIR}/vacsystemupdater.service ${D}${systemd_system_unitdir}/

	# Edit the RHPRO_USER field in the service files
	sed -i 's,@ORIGINAL_USER@,${ORIGINAL_USER},g' ${D}${systemd_system_unitdir}/*.service
}

SYSTEMD_SERVICE_${PN} = "vacsystemupdater.service"

FILES_${PN} += "${systemd_unitdir}/system/*"

