#
# Recipe for vachelper program
#
SRCREV = "${AUTOREV}"

require vachelper-1.0.inc

# Needed for encrypting and zipping output files
RDEPENDS_${PN} += "gnupg zip"

