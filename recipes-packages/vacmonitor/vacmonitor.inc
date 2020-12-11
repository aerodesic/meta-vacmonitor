#
# vacmonitor application (main gui) for vacuum system.
#
SUMMARY = "Visual application for vacuum system."
HOMEPAGE = "http://www.robosity.com"
MAINTAINER = "Gary Oliver <go@robosity.com>"
SECTION = "vacmonitor"
PR = "r1"
LICENSE = "GPLv2"
LIC_FILES_CHKSUM = "file://LICENSE;md5=b234ee4d69f5fce4486a80fdaf4a4263"

SRCREV ?= "${AUTOREV}"
APP_BRANCH ?= ""

inherit systemd

inherit pypi setuptools3 distutils3

S = "${WORKDIR}/git"

SRC_URI = " \
	git://git@github.com/aerodesic/${PN};protocol=ssh;${APP_BRANCH} \
	file://${PN}.service						\
	file://gtkrc-2.0 						\
"

DEPENDS = " \
	wxpython			\
	gtk+3				\
	python3-dbus			\
	python3-pygobject 		\
	intltool-native			\
	python3-distutils-extra-native	\
	python3-networkmanager		\
"

TIMEZONES = " \
	tzdata			\
	tzdata-misc 		\
	tzdata-posix 		\
	tzdata-right 		\
	tzdata-africa 		\
	tzdata-americas 	\
	tzdata-antarctica 	\
	tzdata-arctic 		\
	tzdata-asia 		\
	tzdata-atlantic 	\
	tzdata-australia 	\
	tzdata-europe 		\
	tzdata-pacific 		\
"

RDEPENDS_${PN} = " \
	wxpython			\
	python3				\
	python3-codecs			\
	cantarell-fonts			\
	vachelper			\
	python3-configargparse		\
	python3-dbus			\
	python3-json			\
	python3-pygobject 		\
	python3-pytz			\
	${TIMEZONES}			\
	vacsystemupdater		\
	python3-networkmanager		\
"

# Pick the appropriate service file
SYSTEMD_SERVICE_${PN} = "${PN}.service"

PYTHON_FLAGS = "`python-config --cflags --ldflags`"

do_install_append() {
	install -m 0755 -d ${D}${PYTHON_SITEPACKAGES_DIR}
	echo "${PN}" > ${D}${PYTHON_SITEPACKAGES_DIR}/${PN}-${PV}.pth
	# Remove .pyc files that have been generated
	rm -f "${D}${PYTHON_SITEPACKAGES_DIR}/${PN}/*.pyc"

	install -d ${D}${systemd_unitdir}/system
	install -m 0644 ${WORKDIR}/${PN}.service ${D}${systemd_unitdir}/system

	# Edit the ORIGINAL_USER field in the service files
	sed -i 's,@ORIGINAL_USER@,${ORIGINAL_USER},g' ${D}${systemd_unitdir}/system/*.service

	# Install a .gtkrc-2.0 in /home/root
	install -d ${D}/home/root
	install -m 0644 ${WORKDIR}/gtkrc-2.0 ${D}/home/root/.gtkrc-2.0
}

FILES_${PN} = "/usr/share/bitmaps ${datadir} ${systemd_unitdir} ${bindir} ${libdir} /usr/share/locale /home/root"

