/*
 * TipThermoModel.cpp
 *
 *  Created on: 7 Oct 2019
 *      Author: ralim
 */

#include "TipThermoModel.h"
#include "Settings.h"
#include "BSP.h"
#include "power.hpp"
#include "../../configuration.h"
#include "main.hpp"
/*
 * The hardware is laid out  as a non-inverting op-amp
 * There is a pullup of 39k(TS100) from the +ve input to 3.9V (1M pulup on TS100)
 *
 * The simplest case to model this, is to ignore the pullup resistors influence, and assume that its influence is mostly constant
 * -> Tip resistance *does* change with temp, but this should be much less than the rest of the system.
 *
 * When a thermocouple is equal temperature at both sides (hot and cold junction), then the output should be 0uV
 * Therefore, by measuring the uV when both are equal, the measured reading is the offset value.
 * This is a mix of the pull-up resistor, combined with tip manufacturing differences.
 *
 * All of the thermocouple readings are based on this expired patent
 * - > https://patents.google.com/patent/US6087631A/en
 *
 * This was bought to my attention by <Kuba Sztandera>
 */

uint32_t TipThermoModel::convertTipRawADCTouV(uint16_t rawADC) {
	// This takes the raw ADC samples, converts these to uV
	// Then divides this down by the gain to convert to the uV on the input to the op-amp (A+B terminals)
	// Then remove the calibration value that is stored as a tip offset
	uint32_t vddRailmVX10 = 33000;	//The vreg is +-2%, but we have no higher accuracy available
	// 4096 * 8 readings for full scale
	// Convert the input ADC reading back into mV times 10 format.
	uint32_t rawInputmVX10 = (rawADC * vddRailmVX10) / (4096 * 8);

	uint32_t valueuV = rawInputmVX10 * 100;	// shift into uV
	//Now to divide this down by the gain
	valueuV /= OP_AMP_GAIN_STAGE;

	if (systemSettings.CalibrationOffset) {
		//Remove uV tipOffset
		if (valueuV >= systemSettings.CalibrationOffset)
			valueuV -= systemSettings.CalibrationOffset;
		else
			valueuV = 0;
	}

	return valueuV;
}

uint32_t TipThermoModel::convertTipRawADCToDegC(uint16_t rawADC) {
	return convertuVToDegC(convertTipRawADCTouV(rawADC));
}
#ifdef ENABLED_FAHRENHEIT_SUPPORT
uint32_t TipThermoModel::convertTipRawADCToDegF(uint16_t rawADC) {
	return convertuVToDegF(convertTipRawADCTouV(rawADC));
}
#endif

//Table that is designed to be walked to find the best sample for the lookup

//Extrapolate between two points
// [x1, y1] = point 1
// [x2, y2] = point 2
//  x = input value
// output is x's extrapolated y value
int32_t LinearInterpolate(int32_t x1, int32_t y1, int32_t x2, int32_t y2, int32_t x) {
	return y1 + (((((x - x1) * 1000) / (x2 - x1)) * (y2 - y1))) / 1000;
}
#ifdef TEMP_uV_LOOKUP_HAKKO
const uint16_t uVtoDegC[] = { //
		//
				0, 0,	//
				266, 10,	//
				522, 20,	//
				770, 30,	//
				1010, 40,	//
				1244, 50,	//
				1473, 60,	//
				1697, 70,	//
				1917, 80,	//
				2135, 90,	//
				2351, 100,	//
				2566, 110,	//
				2780, 120,	//
				2994, 130,	//
				3209, 140,	//
				3426, 150,	//
				3644, 160,	//
				3865, 170,	//
				4088, 180,	//
				4314, 190,	//
				4544, 200,	//
				4777, 210,	//
				5014, 220,	//
				5255, 230,	//
				5500, 240,	//
				5750, 250,	//
				6003, 260,	//
				6261, 270,	//
				6523, 280,	//
				6789, 290,	//
				7059, 300,	//
				7332, 310,	//
				7609, 320,	//
				7889, 330,	//
				8171, 340,	//
				8456, 350,	//
				8742, 360,	//
				9030, 370,	//
				9319, 380,	//
				9607, 390,	//
				9896, 400,	//
				10183, 410,	//
				10468, 420,	//
				10750, 430,	//
				11029, 440,	//
				11304, 450,	//
				11573, 460,	//
				11835, 470,	//
				12091, 480,	//
				12337, 490,	//
				12575, 500,	//

		};
#endif

#ifdef TEMP_uV_LOOKUP_TS80

const uint16_t uVtoDegC[] = {	//
		//

        2337 , 0,		//
		3008 , 10,		//
		3693 , 20,		//
		4390 , 30,		//
		5101 , 40,		//
		5825 , 50,		//
		6562 , 60,		//
		7312 , 70,		//
		8076 , 80,		//
		8852 , 90,		//
		9642 , 100,		//
		10445 , 110,		//
		11261 , 120,		//
		12090 , 130,		//
		12932 , 140,		//
		13787 , 150,		//
		14656 , 160,		//
		15537 , 170,		//
		16432 , 180,		//
		17340 , 190,		//
		18261 , 200,		//
		19195 , 210,		//
		20143 , 220,		//
		21103 , 230,		//
		22077 , 240,		//
		23063 , 250,		//
		24063 , 260,		//
		25076 , 270,		//
		26102 , 280,		//
		27142 , 290,		//
		28194 , 300,		//
		29260 , 310,		//
		30339 , 320,		//
		31430 , 330,		//
		32535 , 340,		//
		33654 , 350,		//
		34785 , 360,		//
		35929 , 370,		//
		37087 , 380,		//
		38258 , 390,		//
		39441 , 400,		//
		40638 , 410,		//
		41849 , 420,		//
		43072 , 430,		//
		44308 , 440,		//
		45558 , 450,		//
		46820 , 460,		//
		48096 , 470,		//
		49385 , 480,		//
		50687 , 490,		//
		52003 , 500,		//
		};
#endif
uint32_t TipThermoModel::convertuVToDegC(uint32_t tipuVDelta) {
	if (tipuVDelta) {
		int noItems = sizeof(uVtoDegC) / (2 * sizeof(uint16_t));
		for (int i = 1; i < (noItems - 1); i++) {
			//If current tip temp is less than current lookup, then this current lookup is the higher point to interpolate
			if (tipuVDelta < uVtoDegC[i * 2]) {
				return LinearInterpolate(uVtoDegC[(i - 1) * 2], uVtoDegC[((i - 1) * 2) + 1], uVtoDegC[i * 2], uVtoDegC[(i * 2) + 1], tipuVDelta);
			}
		}
		return LinearInterpolate(uVtoDegC[(noItems - 2) * 2], uVtoDegC[((noItems - 2) * 2) + 1], uVtoDegC[(noItems - 1) * 2], uVtoDegC[((noItems - 1) * 2) + 1], tipuVDelta);
	}
	return 0;
}

#ifdef ENABLED_FAHRENHEIT_SUPPORT
uint32_t TipThermoModel::convertuVToDegF(uint32_t tipuVDelta) {
	return convertCtoF(convertuVToDegC(tipuVDelta));
}

uint32_t TipThermoModel::convertCtoF(uint32_t degC) {
	//(Y °C × 9/5) + 32 =Y°F
	return (32 + ((degC * 9) / 5));
}

uint32_t TipThermoModel::convertFtoC(uint32_t degF) {
	//(Y°F − 32) × 5/9 = Y°C
	if (degF < 32) {
		return 0;
	}
	return ((degF - 32) * 5) / 9;
}
#endif

uint32_t TipThermoModel::getTipInC(bool sampleNow) {
	int32_t currentTipTempInC = TipThermoModel::convertTipRawADCToDegC(getTipRawTemp(sampleNow));
	currentTipTempInC += getHandleTemperature() / 10; //Add handle offset
	// Power usage indicates that our tip temp is lower than our thermocouple temp.
	// I found a number that doesn't unbalance the existing PID, causing overshoot.
	// This could be tuned in concert with PID parameters...
	currentTipTempInC -= x10WattHistory.average() / 25;
	if (currentTipTempInC < 0)
		return 0;
	return currentTipTempInC;
}
#ifdef ENABLED_FAHRENHEIT_SUPPORT
uint32_t TipThermoModel::getTipInF(bool sampleNow) {
	uint32_t currentTipTempInF = getTipInC(sampleNow);
	currentTipTempInF = convertCtoF(currentTipTempInF);
	return currentTipTempInF;
}
#endif

uint32_t TipThermoModel::getTipMaxInC() {
	uint32_t maximumTipTemp = TipThermoModel::convertTipRawADCToDegC(0x7FFF - (80 * 5)); //back off approx 5 deg c from ADC max
	maximumTipTemp += getHandleTemperature() / 10; //Add handle offset
	return maximumTipTemp - 1;
}
