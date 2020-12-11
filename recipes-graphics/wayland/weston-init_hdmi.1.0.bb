SUMMARY = "Startup script and systemd unit file for the Weston Wayland compositor"

LICENSE = "GPLv2+"
LIC_FILES_CHKSUM = "file://${COREBASE}/meta/COPYING.MIT;md5=3da9cfbcb788c80a0384361b4de20420"

SRC_URI = " \
    file://init \
    file://weston.service \
    file://weston-start \
    file://weston-wait-psplash \
    file://default.weston-systemd \
    file://default.weston \
    file://weston.ini \
    file://vacmonitor_logo.png \
"

S = "${WORKDIR}"


do_install() {
	install -Dm755 ${WORKDIR}/init ${D}/${sysconfdir}/init.d/weston
	install -Dm0644 ${WORKDIR}/weston.service ${D}${systemd_system_unitdir}/weston.service

	# Install weston-start script
	install -Dm755 ${WORKDIR}/weston-start ${D}${bindir}/weston-start
	sed -i 's,@DATADIR@,${datadir},g' ${D}${bindir}/weston-start
	sed -i 's,@LOCALSTATEDIR@,${localstatedir},g' ${D}${bindir}/weston-start

	# Add function to wait for frame buffer to appear
	install -Dm755 ${WORKDIR}/weston-wait-psplash     ${D}/${sbindir}/weston-wait-psplash

	# Add weston default config (launch on /dev/fb1)
	install -Dm 0755 ${WORKDIR}/default.weston ${D}${sysconfdir}/default/weston
	install -Dm 0755 ${WORKDIR}/default.weston-systemd ${D}${sysconfdir}/default/weston-systemd

	# Add weston.ini to etc
	install -Dm 0755 ${WORKDIR}/weston.ini ${D}${sysconfdir}

        # Add bitmap to /usr/share/bitmaps
        install -m 755 -d ${D}/usr/share/bitmaps
        install -m 644 ${WORKDIR}/vacmonitor_logo.png ${D}/usr/share/bitmaps/
}

# inherit allarch update-rc.d distro_features_check systemd
inherit allarch update-rc.d features_check systemd

# rdepends on weston which depends on virtual/egl
REQUIRED_DISTRO_FEATURES = "opengl"

RDEPENDS_${PN} = "weston kbd bash fb1"

INITSCRIPT_NAME = "weston"
INITSCRIPT_PARAMS = "start 9 5 2 . stop 20 0 1 6 ."

SYSTEMD_SERVICE_${PN} = "weston.service"

FILES_${PN} += "${sbindir}/* ${sysconfdir}/default /usr/share/bitmaps/"
