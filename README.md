(work in progress)
# DOORS

"Doors" is a local, centralized system for controlling and managing a set of automatic doors.

Each door is controlled by an STM32F756 board, and includes a touch display for the usual stuff - entering a passcode or buzzing a tenant.

A Linux board (Beaglebone Green) serves as a central hub, managing multiple door units over an I2C bus.

This hub unit also liaises between the door units and a separate set of client or "intercom" units, connected to the hub over a local network.
