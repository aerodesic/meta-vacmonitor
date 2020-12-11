SUMMARY = "A very basic Wayland image for vacuum test system"
MAINTAINER = "Gary Oliver <go@robosity.com>"

IMAGE_FEATURES += "splash package-management ssh-server-dropbear hwcodecs"

IMAGE_INSTALL_append += " systemd-network-enable"
IMAGE_INSTALL_append += " psplash"
# IMAGE_INSTALL_append += " dtc"
# IMAGE_INSTALL_append += " cups"
# IMAGE_INSTALL_append += " cups-filters"
IMAGE_INSTALL_append += " poppler"
# IMAGE_INSTALL_append += " ghostscript"
IMAGE_INSTALL_append += " e2fsprogs"
IMAGE_INSTALL_append += " e2fsprogs-resize2fs"
# IMAGE_INSTALL_append += " hplip"
IMAGE_INSTALL_append += " vacsystemupdater-systemd"

IMAGE_INSTALL_append += " vacmonitor vacmonitor-version"

LICENSE = "MIT"

UPDATE_MANAGEMENT_CACHE = "builder-package-cache"

# inherit core-image distro_features_check update-management
inherit core-image features_check update-management

REQUIRED_DISTRO_FEATURES = "wayland"

CORE_IMAGE_EXTRA_INSTALL = "weston weston-init weston-examples gtk+3-demo clutter-1.0-examples sudo"
CORE_IMAGE_EXTRA_INSTALL += "${@bb.utils.contains('DISTRO_FEATURES', 'x11', 'weston-xwayland matchbox-terminal', '', d)}"

