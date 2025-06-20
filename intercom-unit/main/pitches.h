/*
 * pitches.h
 *
 *  Created on: Mar 1, 2025
 *      Author: mickey
 */

#ifndef INC_PITCHES_H_
#define INC_PITCHES_H_

#include "hal/ledc_types.h"
#include "soc/soc.h"

// assuming base clock frequency is that of the clock source
#define TCLOCK_HZ REF_CLK_FREQ 

// determining the divider required to achieve a target frequency from the base
#define PITCH_PERIOD(pitch_hz) (uint32_t)((TCLOCK_HZ) / (float)(pitch_hz))

// hz values of note pitches
#define C0 PITCH_PERIOD(16.35)
#define Db0 PITCH_PERIOD(17.32)
#define D0 PITCH_PERIOD(18.35)
#define Eb0 PITCH_PERIOD(19.45)
#define E0 PITCH_PERIOD(20.6)
#define F0 PITCH_PERIOD(21.83)
#define Gb0 PITCH_PERIOD(23.12)
#define G0 PITCH_PERIOD(24.5)
#define Ab0 PITCH_PERIOD(25.96)
#define A0 PITCH_PERIOD(27.5)
#define Bb0 PITCH_PERIOD(29.14)
#define B0 PITCH_PERIOD(30.87)

#define C1 PITCH_PERIOD(32.7)
#define Db1 PITCH_PERIOD(34.65)
#define D1 PITCH_PERIOD(36.71)
#define Eb1 PITCH_PERIOD(38.89)
#define E1 PITCH_PERIOD(41.2)
#define F1 PITCH_PERIOD(43.65)
#define Gb1 PITCH_PERIOD(46.25)
#define G1 PITCH_PERIOD(49)
#define Ab1 PITCH_PERIOD(51.91)
#define A1 PITCH_PERIOD(55)
#define Bb1 PITCH_PERIOD(58.27)
#define B1 PITCH_PERIOD(61.74)

#define C2 PITCH_PERIOD(65.41)
#define Db2 PITCH_PERIOD(69.3)
#define D2 PITCH_PERIOD(73.42)
#define Eb2 PITCH_PERIOD(77.78)
#define E2 PITCH_PERIOD(82.41)
#define F2 PITCH_PERIOD(87.31)
#define Gb2 PITCH_PERIOD(92.5)
#define G2 PITCH_PERIOD(98)
#define Ab2 PITCH_PERIOD(103.83)
#define A2 PITCH_PERIOD(110)
#define Bb2 PITCH_PERIOD(116.54)
#define B2 PITCH_PERIOD(123.47)

#define C3 PITCH_PERIOD(130.81)
#define Db3 PITCH_PERIOD(138.59)
#define D3 PITCH_PERIOD(146.83)
#define Eb3 PITCH_PERIOD(155.56)
#define E3 PITCH_PERIOD(164.81)
#define F3 PITCH_PERIOD(174.61)
#define Gb3 PITCH_PERIOD(185)
#define G3 PITCH_PERIOD(196)
#define Ab3 PITCH_PERIOD(207.65)
#define A3 PITCH_PERIOD(220)
#define Bb3 PITCH_PERIOD(233.08)
#define B3 PITCH_PERIOD(246.94)

#define C4 PITCH_PERIOD(261.63)
#define Db4 PITCH_PERIOD(277.18)
#define D4 PITCH_PERIOD(293.66)
#define Eb4 PITCH_PERIOD(311.13)
#define E4 PITCH_PERIOD(329.63)
#define F4 PITCH_PERIOD(349.23)
#define Gb4 PITCH_PERIOD(369.99)
#define G4 PITCH_PERIOD(392)
#define Ab4 PITCH_PERIOD(415.3)
#define A4 PITCH_PERIOD(440)
#define Bb4 PITCH_PERIOD(466.16)
#define B4 PITCH_PERIOD(493.88)

#define C5 PITCH_PERIOD(523.25)
#define Db5 PITCH_PERIOD(554.37)
#define D5 PITCH_PERIOD(587.33)
#define Eb5 PITCH_PERIOD(622.25)
#define E5 PITCH_PERIOD(659.25)
#define F5 PITCH_PERIOD(698.46)
#define Gb5 PITCH_PERIOD(739.99)
#define G5 PITCH_PERIOD(783.99)
#define Ab5 PITCH_PERIOD(830.61)
#define A5 PITCH_PERIOD(880)
#define Bb5 PITCH_PERIOD(932.33)
#define B5 PITCH_PERIOD(987.77)

#define C6 PITCH_PERIOD(1046.5)
#define Db6 PITCH_PERIOD(1108.73)
#define D6 PITCH_PERIOD(1174.66)
#define Eb6 PITCH_PERIOD(1244.51)
#define E6 PITCH_PERIOD(1318.51)
#define F6 PITCH_PERIOD(1396.61)
#define Gb6 PITCH_PERIOD(1479.98)
#define G6 PITCH_PERIOD(1567.98)
#define Ab6 PITCH_PERIOD(1661.22)
#define A6 PITCH_PERIOD(1760)
#define Bb6 PITCH_PERIOD(1864.66)
#define B6 PITCH_PERIOD(1975.53)

#endif /* INC_PITCHES_H_ */
