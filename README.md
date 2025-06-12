(work in progress)
# DOORS

"Doors" is a local, centralized system for controlling and managing a set of automatic doors.

Each door is controlled by an STM32F756 board, and includes a touch display for the usual stuff - entering a passcode or buzzing a tenant.

A Linux SBC (Beaglebone Green) serves as a central hub, managing multiple door units over an I2C bus.

This hub unit also liaises between the door units and a separate set of (ESP32) client or "intercom" units, connected to the hub over a local network.

Further responsibilities of the hub unit include facilitating interactive administration of the system and documenting system-wide events in a central database.
