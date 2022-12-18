#include "AuntyLangton.hpp"
#include <iostream>
#include <vector>
#include <algorithm>
#include <math.h>
using namespace std;
#define CELLS 20736
#define DISPLAY_OFFSET_X 35
#define DISPLAY_OFFSET_Y 28
#define DISPLAY_SIZE_XY 135
#define X_POSITION 0
#define Y_POSITION 1
#define DIRECTION 2
#define PIXEL_BRIGHTNESS 140
#define INITIAL_RESOLUTION_KNOB_POSITION 5
#define LEFT -90
#define RIGHT 90

/*
Big thanks to..
Andrew Belt of VCV Rack - https://vcvrack.com/
Jeremy Wentworth of JW Modules - http://jeremywentworth.com
.. for providing the open source structure to build upon.
*/

// TODO 3 - Try dual outputs for one axis using a shepard tone
// TODO 4 - Add a splash screen for when the pluginInstance is initialized (done) and then remove it.
// TODO 5 (done) - X or Y out only fires if ant is changing on that axis?
//			- Remove bi-directional switches for X and Y out. DONE
//			- Change "Clock Out" to "Gate" DONE
//			- Add Three-directional switch next to gate with options "X/Y", "X" or "Y" CHECK THIS.
// TODO 6 - Either change all arrays to vectors or change ant and shadow ant direction dynamic arrays to static to HISTORY_AMOUNT
// TODO 7 - override onChange event for knobs to display knob values?

/*
Personal dev notes:

Compile:
RACK_DIR=/Users/my/Documents/Rack-SDK make

Compile for AddressSanitizer:
Add following to Makefile: CXXFLAGS += -g -fsanitize=address

Run with: DYLD_INSERT_LIBRARIES=/Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/lib/clang/11.0.0/lib/darwin/libclang_rt.asan_osx_dynamic.dylib ./Rack
*/

struct MusicalAnt : Module, QuantizeUtils{
	enum ParamIds {
		CLOCK_PARAM,
		SCALE_KNOB_X_PARAM,
		SCALE_KNOB_Y_PARAM,
		SCALE_KNOB_SHADOW_X_PARAM,
		SCALE_KNOB_SHADOW_Y_PARAM,
		NOTE_KNOB_X_PARAM,
		NOTE_KNOB_Y_PARAM,
		NOTE_KNOB_SHADOW_X_PARAM,
		NOTE_KNOB_SHADOW_Y_PARAM,
		OCTAVE_KNOB_X_PARAM,
		OCTAVE_KNOB_Y_PARAM,
		OCTAVE_KNOB_SHADOW_X_PARAM,
		OCTAVE_KNOB_SHADOW_Y_PARAM,
		SKIP_PARAM,
		EFFECT_KNOB_PARAM,
		SHADOW_ANT_ON_PARAM,
		SIDE_LENGTH_PARAM,
		LOOPMODE_SWITCH_PARAM,
		LOOP_LENGTH_PARAM,
		VOCT_INVERT_X_PARAM,
		VOCT_INVERT_Y_PARAM,
		AUNTYLANGBUTTON_PARAM,
		NUM_PARAMS
	};
	enum InputIds {
		EXT_CLOCK_INPUT,
		PITCH_INPUT,
		NUM_INPUTS
	};
	enum OutputIds {
		GATE_OUTPUT,
		GATE_OUTPUT_LEFT,
		GATE_OUTPUT_RIGHT,
		VOCT_OUTPUT_X,
		VOCT_OUTPUT_Y,
		VOCT_OUTPUT_SHADOW_X,
		VOCT_OUTPUT_SHADOW_Y,
		RND_VOCT_OUTPUT,
		RND_GATE_OUTPUT,
		VOCT_OUTPUT_POLY_OUTPUT,
		NUM_OUTPUTS
	};
	enum LightIds {
		BLINK_LIGHT,
		NUM_LIGHTS
	};

	float phase = 0.0;
	float blinkPhase = 0.0;
	bool loopOn = false;
	int loopLength = 0;
	int backStepsRemaining = 0;
	//int loopIndex = 0;
	dsp::SchmittTrigger clockTrigger;
	int fibo[7] = {8, 13, 21, 34, 55, 89, 144}; // short for Fibonacci (cause I forgot that's why I named it that).
	int sideLength = 0;
	bool currentArrowOfTimeForward = true;
	bool lastArrowOfTimeForward = true;

	// New system representation.

	MusicalSystemState* systemState;
	MusicalInstruction* antBehaviour;


	MusicalAnt() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
		configParam(CLOCK_PARAM, -2.0f, 8.0f, 1.0f, "");
		configParam(OCTAVE_KNOB_X_PARAM, 0.0, 7.0, 2.0, "");
		configParam(NOTE_KNOB_X_PARAM, 0.0, QuantizeUtils::NUM_NOTES-1, QuantizeUtils::NOTE_C, "");
		configParam(SCALE_KNOB_X_PARAM, 0.0, QuantizeUtils::NUM_SCALES-1, QuantizeUtils::MINOR, "");
		configParam(VOCT_INVERT_X_PARAM, 0.0f, 1.0f, 1.0f, "");
		configParam(OCTAVE_KNOB_Y_PARAM, 0.0, 7.0, 2.0, "");
		configParam(NOTE_KNOB_Y_PARAM, 0.0, QuantizeUtils::NUM_NOTES-1, QuantizeUtils::NOTE_C, "");
		configParam(SCALE_KNOB_Y_PARAM, 0.0, QuantizeUtils::NUM_SCALES-1, QuantizeUtils::MINOR, "");
		configParam(VOCT_INVERT_Y_PARAM, 0.0f, 1.0f, 1.0f, "");
		configParam(OCTAVE_KNOB_SHADOW_X_PARAM, 0.0, 7.0, 2.0, "");
		configParam(NOTE_KNOB_SHADOW_X_PARAM, 0.0, QuantizeUtils::NUM_NOTES-1, QuantizeUtils::NOTE_C, "");
		configParam(SCALE_KNOB_SHADOW_X_PARAM, 0.0, QuantizeUtils::NUM_SCALES-1, QuantizeUtils::MINOR, "");
		configParam(OCTAVE_KNOB_SHADOW_Y_PARAM, 0.0, 7.0, 2.0, "");
		configParam(NOTE_KNOB_SHADOW_Y_PARAM, 0.0, QuantizeUtils::NUM_NOTES-1, QuantizeUtils::NOTE_C, "");
		configParam(SCALE_KNOB_SHADOW_Y_PARAM, 0.0, QuantizeUtils::NUM_SCALES-1, QuantizeUtils::MINOR, "");
		configParam(SHADOW_ANT_ON_PARAM, 0.0f, 1.0f, 1.0f, "");
		configParam(EFFECT_KNOB_PARAM, 0.0f, 5.0f, 0.0, "");
		configParam(LOOPMODE_SWITCH_PARAM, 0.0f, 1.0f, 0.0f, "");
		configParam(LOOP_LENGTH_PARAM, 0.0f, 64.0f, 0.0, "");
		configParam(SIDE_LENGTH_PARAM, 0.0f, 6.0f, INITIAL_RESOLUTION_KNOB_POSITION, "");
		configParam(SKIP_PARAM, 0.0f, 32.0f, 0.0f, "");
		configParam(AUNTYLANGBUTTON_PARAM, 0.0f, 1.0f, 0.0f, "");

		sideLength = fibo[INITIAL_RESOLUTION_KNOB_POSITION];

		systemState = new MusicalSystemState();
		antBehaviour = new MusicalInstruction();

		onReset();
	}

	//~MusicalAnt() {
	//}

	void onReset() override {
		
		clearCells();
		int sideLengthOnReset = getSideLength();
		setAntPosition(sideLengthOnReset/2, sideLengthOnReset/2, 0);
		
		setShadowAntPosition(sideLengthOnReset/2, sideLengthOnReset/2, 0);
		
		//loopIndex = 0;
		


		/*if(cellsHistory.size() > 0) {
			cellsHistory.clear();
		}
		if(antVectorHistory.size() > 0) {
			antVectorHistory.clear();
		}
		if(shadowAntVectorHistory.size() > 0) {
			shadowAntVectorHistory.clear();
		}*/
		
		// New system representation.

		systemState->antX = sideLengthOnReset/2;
		systemState->antY = sideLengthOnReset/2;
		systemState->antDirectionDegrees = 0;

		systemState->shadowAntX = sideLengthOnReset/2;
		systemState->shadowAntY = sideLengthOnReset/2;
		systemState->shadowAntDirectionDegrees = 0;
		
	}

	json_t *dataToJson() override {
	

		/*

		json_t *antVectorHistoryJ = json_array();
		int vectorHistoryCellsCount = HISTORY_AMOUNT * 3;
		for (int i = 0; i < vectorHistoryCellsCount; i++) {
			json_t *antVectorHistoryJ = json_integer((int) antVectorHistory.at(i/3).at(i%3));
			json_array_append_new(antVectorHistoryJ, antVectorHistoryJ);
		}
		json_object_set_new(rootJ, "antVectorHistory", antVectorHistoryJ);


		json_t *shadowAntVectorHistoryJ = json_array();
		for (int i = 0; i < vectorHistoryCellsCount; i++) {
			json_t *shadowAntVectorHistoryJ = json_integer((int) shadowAntVectorHistory.at(i/3).at(i%3));
			json_array_append_new(shadowAntVectorHistoryJ, shadowAntVectorHistoryJ);
		}
		json_object_set_new(rootJ, "shadowAntVectorHistory", shadowAntVectorHistoryJ);

		json_t *cellsHistoryJ = json_array();
		int cellsHistoryCount = HISTORY_AMOUNT * CELLS;
		for (int i = 0; i < cellsHistoryCount; i++) {
			json_t *cellsHistoryJ = json_integer((int) cellsHistory.at(i/CELLS).at(i%CELLS));
			json_array_append_new(cellsHistoryJ, cellsHistoryJ);
		}
		json_object_set_new(rootJ, "cellsHistory", cellsHistoryJ);

		*/

		json_t *rootJ = json_object();
		json_t *cellsJ = json_array();
	
		for (int i = 0; i < CELLS; i++) {
			json_t *cellJ = json_integer(systemState->cells.at(i));
			json_array_append_new(cellsJ, cellJ);
		}
		json_object_set_new(rootJ, "cells", cellsJ);
		return rootJ;

		/*
		ToJson TODO
		cellsHistory.clear();
		index settings ?
		*/
		
		
	}

	void dataFromJson(json_t *rootJ) override {

		json_t *cellsJ = json_object_get(rootJ, "cells");
		if (cellsJ) {
			for (int i = 0; i < CELLS; i++) {
				json_t *cellJ = json_array_get(cellsJ, i);
				if (cellJ)
					systemState->cells.at(i) = json_integer_value(cellJ);
			}
		}
	}

	int wrap(int kX, int const kLowerBound, int const kUpperBound) {
    	int range_size = kUpperBound - kLowerBound + 1;
	    if (kX < kLowerBound)
	    	kX += range_size * ((kLowerBound - kX) / range_size + 1);

    	return kLowerBound + (kX - kLowerBound) % range_size;
    }

	bool getLoopOn() {
		return this->loopOn;
	}

	void setLoopOn(bool loopIsOn) {
		this->loopOn = loopIsOn;
	}

	int getLoopLength() {
		return this->loopLength;
	}

	void setLoopLength(int value) {
		this->loopLength = value;
	}


	void setSideLength(int x) {
		sideLength = fibo[wrap(x, 0, getSideLength()-1)];
	}

	int getSideLength() {
		return sideLength;
	}

	void setAntPosition(int x, int y, int direction) {
		systemState->antX = wrap(x, 0, sideLength-1);
		systemState->antY = wrap(y, 0, sideLength-1);
		systemState->antDirectionDegrees = direction;
	}

	void setShadowAntPosition(int x, int y, int direction) {
		systemState->shadowAntX = wrap(x, 0, sideLength-1);
		systemState->shadowAntY = wrap(y, 0, sideLength-1);
		systemState->shadowAntDirectionDegrees = direction;
	}


	int getAntX() {
		return systemState->antX;
	}

	int getAntY() {
		return systemState->antY;
	}

	int getShadowAntX() {
		return systemState->shadowAntX;
	}

	int getShadowAntY() {
		return systemState->shadowAntY;
	}

	int getAntDirection() {
		return systemState->antDirectionDegrees;
	}

	void process(const ProcessArgs &args) override {
		bool loopIsOn = params[LOOPMODE_SWITCH_PARAM].getValue();

		//int currentIndex = getIndex();

		bool gateIn = false;
		int numberSteps = params[SKIP_PARAM].getValue() + 1;

		

		loopLength = params[LOOP_LENGTH_PARAM].getValue() + 1;
		//setLoopLength(loopLength);



		// Looping implementation
		

		if (inputs[EXT_CLOCK_INPUT].isConnected()) {
			// External clock
			if (clockTrigger.process(rescale(inputs[EXT_CLOCK_INPUT].getVoltage(), 0.1f, 2.f, 0.f, 1.f))) {

				if(loopIsOn && (backStepsRemaining >= loopLength*numberSteps)) {
					//loopIndex = currentIndex;
					walkAnt(-1*loopLength*numberSteps);
					///setLoopOn(loopIsOn);
					backStepsRemaining = clamp(backStepsRemaining - loopLength*numberSteps, 0, loopLength*numberSteps);
					
				}
				else {
					walkAnt(numberSteps);
					backStepsRemaining = clamp(backStepsRemaining + numberSteps, 0, loopLength*numberSteps);
				}
			}
			gateIn = clockTrigger.isHigh();
		}
		else {
			// Internal clock
			float clockParam = params[CLOCK_PARAM].getValue();
			if(clockParam > 0.0) {
				float clockTime = powf(2.0f, params[CLOCK_PARAM].getValue());
				phase += clockTime * args.sampleTime;
				if (phase >= 1.0f) {
					phase -= 1.0f;
					if(loopIsOn && (backStepsRemaining >= loopLength*numberSteps)) {
						//loopIndex = currentIndex;
						walkAnt(-1*loopLength*numberSteps);
						//setLoopOn(loopIsOn);
						backStepsRemaining = clamp(backStepsRemaining - loopLength*numberSteps, 0, loopLength*numberSteps);
						
					}
					else {
						walkAnt(numberSteps);
						backStepsRemaining = clamp(backStepsRemaining + numberSteps, 0, loopLength*numberSteps);
					} 
				}
				gateIn = (phase < 0.5f);
			}
		}
		



		// TODO Fix up this var below. May not be needed, or at least needs refactoring
		int tempSideLength = params[SIDE_LENGTH_PARAM].getValue();

		setSideLength(tempSideLength);
		float gateOut = (gateIn ? 10.0f : 0.0f);

		//params[INDEX_PARAM].setValue(currentIndex);

		outputs[GATE_OUTPUT].setVoltage(gateOut);

		outputs[VOCT_OUTPUT_POLY_OUTPUT].setVoltage(!params[VOCT_INVERT_X_PARAM].getValue() ? closestVoltageForX(tempSideLength - getAntX()) : closestVoltageForX(getAntX()), 0);;
		outputs[VOCT_OUTPUT_POLY_OUTPUT].setVoltage(!params[VOCT_INVERT_Y_PARAM].getValue() ? closestVoltageForY(tempSideLength - getAntY()) : closestVoltageForY(getAntY()), 1);;
		if(params[SHADOW_ANT_ON_PARAM].getValue() == 1.0) {
			outputs[VOCT_OUTPUT_POLY_OUTPUT].setVoltage(closestVoltageForShadowX(getShadowAntX()), 2);
			outputs[VOCT_OUTPUT_POLY_OUTPUT].setVoltage(closestVoltageForShadowY(getShadowAntY()), 3);
			// Don't forget to set the number of output channels
			outputs[VOCT_OUTPUT_POLY_OUTPUT].setChannels(4);
		}
		else {
			outputs[VOCT_OUTPUT_POLY_OUTPUT].setChannels(2);
		}

		lights[BLINK_LIGHT].value = gateIn ? 1.0f : 0.0f;
	}

	void clearCells() {
		for(unsigned int i=0;i<CELLS;i++){
			systemState->cells.at(i) = false;
		}
	}

	float closestVoltageForX(int cellFromBottom){
		int octave = params[OCTAVE_KNOB_X_PARAM].getValue();
		int rootNote = params[NOTE_KNOB_X_PARAM].getValue();
		int scale = params[SCALE_KNOB_X_PARAM].getValue();
		return closestVoltageInScale(octave + (cellFromBottom * 0.0833), rootNote, scale);
	}

	float closestVoltageForShadowX(int cellFromBottom){
		int octave = params[OCTAVE_KNOB_SHADOW_X_PARAM].getValue();
		int rootNote = params[NOTE_KNOB_SHADOW_X_PARAM].getValue();
		int scale = params[SCALE_KNOB_SHADOW_X_PARAM].getValue();
		return closestVoltageInScale(octave + (cellFromBottom * 0.0833), rootNote, scale);
	}

	float closestVoltageForY(int cellFromBottom){
		int octave = params[OCTAVE_KNOB_Y_PARAM].getValue();
		int rootNote = params[NOTE_KNOB_Y_PARAM].getValue();
		int scale = params[SCALE_KNOB_Y_PARAM].getValue();
		return closestVoltageInScale(octave + (cellFromBottom * 0.0833), rootNote, scale);
	}


	float closestVoltageForShadowY(int cellFromBottom){
		int octave = params[OCTAVE_KNOB_SHADOW_Y_PARAM].getValue();
		int rootNote = params[NOTE_KNOB_SHADOW_Y_PARAM].getValue();
		int scale = params[SCALE_KNOB_SHADOW_Y_PARAM].getValue();
		return closestVoltageInScale(octave + (cellFromBottom * 0.0833), rootNote, scale);
	}
 
	void setCellOn(int cellX, int cellY, bool on){
		if(cellX >= 0 && cellX < getSideLength() && 
		   cellY >=0 && cellY < getSideLength()){
			systemState->cells.at(iFromXY(cellX, cellY)) = on;
		}
	}

	void toggleCellState(int cellX, int cellY){
		setCellOn(cellX, cellY, !getCellState(cellX,cellY));
	}

	bool getCellState(int cellX, int cellY){
		return systemState->cells.at(iFromXY(cellX, cellY));
	}

	void setCellOnByDisplayPos(float displayX, float displayY, bool on){
		float pixelSize = 0.9f * (static_cast<float>(DISPLAY_SIZE_XY) / static_cast<float>(getSideLength()));
		setCellOn(displayX / pixelSize, displayY / pixelSize, on);
	}

	bool getCellStateByDisplayPos(float displayX, float displayY){
		float pixelSize = 0.9f * (static_cast<float>(DISPLAY_SIZE_XY) / (float) getSideLength());
		return getCellState(displayX / pixelSize, displayY / pixelSize);
	}

	int iFromXY(int cellX, int cellY){
		return cellX + cellY * getSideLength();
	}

	int xFromI(int cellI){
		return cellI % getSideLength();
	}

	int yFromI(int cellI){
		return cellI / getSideLength();
	}

	int mod(int a, int b)
	{
	    int r = a % b;
	    return r < 0 ? r + b : r;
	}

	int turnDegrees(int degrees) {
		return wrap(degrees, 0, 359);
	}

	void stepAnt(bool arrowOfTime){
		

		currentArrowOfTimeForward = arrowOfTime;
		// Ant.

		int currPositionX = systemState->antX;
		int currPositionY = systemState->antY;
		bool currentCellState = getCellState(currPositionX, currPositionY);
		int currentDirection = systemState->antDirectionDegrees;
		int antRotation = 0;
		int newDirection = 0;
		
		if (currentArrowOfTimeForward != lastArrowOfTimeForward) {
			newDirection = turnDegrees(currentDirection + 180);
		}
		else {
			if(currentCellState == true) {
				antRotation = antBehaviour->getOnLightInstruction();
				newDirection = turnDegrees(currentDirection + antRotation);
			}
			else if(currentCellState == false) {
				antRotation = antBehaviour->getOnDarkInstruction();
				newDirection = turnDegrees(currentDirection + antRotation);
			}
			toggleCellState(currPositionX, currPositionY);
		}


		//Move the ant and set it's new direction
		switch(newDirection) {
			case 0 : {
				// Ant goes up
				setAntPosition(currPositionX, currPositionY - 1, newDirection);
				break;
			}
			case 90 : {
				// Ant goes right
				setAntPosition(currPositionX + 1, currPositionY, newDirection);
				break;
			}
			case 180 : {
				// Ant goes down
				setAntPosition(currPositionX, currPositionY + 1, newDirection);
				break;
			}
			case 270 : {
				// Ant goes left
				setAntPosition(currPositionX - 1, currPositionY, newDirection);
				break;
			}
		}

		//Shadow Ant

		bool shadowAntOn = (bool) !params[SHADOW_ANT_ON_PARAM].getValue();

		if (shadowAntOn) {
			currPositionX = systemState->shadowAntX;
			currPositionY = systemState->shadowAntY;
			currentCellState = getCellState(currPositionX, currPositionY);
			currentDirection = systemState->shadowAntDirectionDegrees;
			
			antRotation = 0;
			newDirection = 0;

			if (currentArrowOfTimeForward != lastArrowOfTimeForward) {
				newDirection = turnDegrees(currentDirection + 180);
			}
			else {
				if(currentCellState == true) {
					antRotation = antBehaviour->getOnLightInstruction();
					newDirection = turnDegrees(currentDirection + antRotation);
				}
				else if(currentCellState == false) {
					antRotation = antBehaviour->getOnDarkInstruction();
					newDirection = turnDegrees(currentDirection + antRotation);
				}
				toggleCellState(currPositionX, currPositionY);
			}
			

			//Move the shadowAnt and set it's new direction
			switch(newDirection) {
				case 90 : {
					// shadowAnt goes up
					setShadowAntPosition(currPositionX, currPositionY - 1, newDirection);
					break;
				}
				case 180 : {
					// shadowAnt goes right
					setShadowAntPosition(currPositionX + 1, currPositionY, newDirection);
					break;
				}
				case 270 : {
					// shadowAnt goes down
					setShadowAntPosition(currPositionX, currPositionY + 1, newDirection);
					break;
				}
				case 0 : {
					// shadowAnt goes left
					setShadowAntPosition(currPositionX - 1, currPositionY, newDirection);
					break;
				}
			}
			// Update outputs
			outputs[VOCT_OUTPUT_SHADOW_X].setVoltage(closestVoltageForShadowX(getShadowAntX()));
			outputs[VOCT_OUTPUT_SHADOW_Y].setVoltage(closestVoltageForShadowY(getShadowAntY()));
		}

		int tempSideLength = params[SIDE_LENGTH_PARAM].getValue();
		outputs[VOCT_OUTPUT_X].setVoltage(!params[VOCT_INVERT_X_PARAM].getValue() ? closestVoltageForX(tempSideLength - getAntX()) : closestVoltageForX(getAntX()));
		outputs[VOCT_OUTPUT_Y].setVoltage(!params[VOCT_INVERT_Y_PARAM].getValue() ? closestVoltageForY(tempSideLength - getAntY()) : closestVoltageForY(getAntY()));
		
		//Left turn
		if(antRotation == LEFT) {
			outputs[GATE_OUTPUT_LEFT].setVoltage(10.0f);
			outputs[GATE_OUTPUT_RIGHT].setVoltage(0.0f);
		}
		//Right turn 
		else if(antRotation == RIGHT) {
			outputs[GATE_OUTPUT_RIGHT].setVoltage(10.0f);
			outputs[GATE_OUTPUT_LEFT].setVoltage(0.0f);
		}

		lastArrowOfTimeForward = currentArrowOfTimeForward;
	}

	void walkAnt(int steps) {
		int paces;
		if(steps < 0) {
			currentArrowOfTimeForward = false;
			paces = steps * -1;
		}
		else {
			currentArrowOfTimeForward = true;
			paces = steps;
		}

		for(int i = 0; i < paces; i++) {
			stepAnt(currentArrowOfTimeForward);
		}
	}

	// For more advanced Module features, read Rack's engine.hpp header file
	// - dataToJson, dataFromJson: serialization of internal data
	// - onSampleRateChange: event triggered by a change of sample rate
	// - onReset, onRandomize, onCreate, onDelete: implements special behavior when user clicks these from the context menu
};


/*void MusicalAnt::process(const ProcessArgs &args) {


	bool loopIsOn = params[LOOPMODE_SWITCH_PARAM].getValue();

	//int currentIndex = getIndex();

	bool gateIn = false;
	int numberSteps = params[SKIP_PARAM].getValue() + 1;

	

	loopLength = (params[LOOP_LENGTH_PARAM].getValue() + 1);
	//setLoopLength(loopLength);



	// Looping implementation
	

	if (inputs[EXT_CLOCK_INPUT].isConnected()) {
		// External clock
		if (clockTrigger.process(rescale(inputs[EXT_CLOCK_INPUT].getVoltage(), 0.1f, 2.f, 0.f, 1.f))) {

			if(loopIsOn && (backStepsRemaining >= loopLength*numberSteps)) {
				//loopIndex = currentIndex;
				walkAnt(-1*loopLength*numberSteps);
				///setLoopOn(loopIsOn);
				backStepsRemaining = clamp(backStepsRemaining - loopLength*numberSteps, 0, loopLength*numberSteps);
				
			}
			else {
				walkAnt(numberSteps);
				backStepsRemaining = clamp(backStepsRemaining + numberSteps, 0, loopLength*numberSteps);
			}
		}
		gateIn = clockTrigger.isHigh();
	}
	else {
		// Internal clock
		float clockTime = powf(2.0f, params[CLOCK_PARAM].getValue());
		phase += clockTime * args.sampleTime;
		if (phase >= 1.0f) {
			phase -= 1.0f;
			if(loopIsOn && (backStepsRemaining >= loopLength*numberSteps)) {
				//loopIndex = currentIndex;
				walkAnt(-1*loopLength*numberSteps);
				//setLoopOn(loopIsOn);
				backStepsRemaining = clamp(backStepsRemaining - loopLength*numberSteps, 0, loopLength*numberSteps);
				
			}
			else {
				walkAnt(numberSteps);
				backStepsRemaining = clamp(backStepsRemaining + numberSteps, 0, loopLength*numberSteps);
			} 
		}

		gateIn = (phase < 0.5f);
	}
	



	// TODO Fix up this var below. May not be needed, or at least needs refactoring
	int tempSideLength = params[SIDE_LENGTH_PARAM].getValue();

	setSideLength(tempSideLength);
	float gateOut = (gateIn ? 10.0f : 0.0f);

	//params[INDEX_PARAM].setValue(currentIndex);


	outputs[GATE_OUTPUT].setVoltage(gateOut);

	outputs[VOCT_OUTPUT_POLY_OUTPUT].setVoltage(!params[VOCT_INVERT_X_PARAM].getValue() ? closestVoltageForX(tempSideLength - getAntX()) : closestVoltageForX(getAntX()), 0);;
	outputs[VOCT_OUTPUT_POLY_OUTPUT].setVoltage(!params[VOCT_INVERT_Y_PARAM].getValue() ? closestVoltageForY(tempSideLength - getAntY()) : closestVoltageForY(getAntY()), 1);;
	if(!params[SHADOW_ANT_ON_PARAM].getValue()) {
		outputs[VOCT_OUTPUT_POLY_OUTPUT].setVoltage(closestVoltageForShadowX(getShadowAntX()), 2);
		outputs[VOCT_OUTPUT_POLY_OUTPUT].setVoltage(closestVoltageForShadowY(getShadowAntY()), 3);
		// Don't forget to set the number of output channels
		outputs[VOCT_OUTPUT_POLY_OUTPUT].setChannels(4);
	}
	else {
		outputs[VOCT_OUTPUT_POLY_OUTPUT].setChannels(2);
	}

	//lights[BLINK_LIGHT].value = gateIn ? 1.0f : 0.0f;

}*/

struct ModuleDisplay : Widget, Logos {
	MusicalAnt *module;
	bool currentlyTurningOn = false;
	float initX = 0;
	float initY = 0;
	float dragX = 0;
	float dragY = 0;

	ModuleDisplay(){}


	void onButton(const event::Button &e) override {
		if (e.action == GLFW_PRESS && e.button == GLFW_MOUSE_BUTTON_LEFT) {
			if(module) {
				e.consume(this);
				// e.target = this;
				initX = e.pos.x;
				initY = e.pos.y;
				if((0 < initX) && (initX < DISPLAY_SIZE_XY) && (0 < initY) && (initY < DISPLAY_SIZE_XY)) {
					currentlyTurningOn = !module->getCellStateByDisplayPos(initX, initY);
					module->setCellOnByDisplayPos(initX, initY, currentlyTurningOn);
				}
			}
		}
	}
	
	/* mousePos not available in Rack2 SDK
	void onDragStart(const event::DragStart &e) override {
		if(module) {
			dragX = APP->scene->rack->mousePos.x;
			dragY = APP->scene->rack->mousePos.y;
		}
	}

	void onDragMove(const event::DragMove &e) override {
		float newDragX = APP->scene->rack->mousePos.x;
		float newDragY = APP->scene->rack->mousePos.y;
		if(module) {
			module->setCellOnByDisplayPos(initX+(newDragX-dragX), initY+(newDragY-dragY), currentlyTurningOn);
		}
	}
	*/

	void draw(const DrawArgs &draw) override {

		int x = 0;
		int y = 0;
			if(module) {
			int drawSideLength = module->getSideLength();
			std::vector<bool> cells;
			cells.resize(CELLS, false);

			float pixelSize = 0.9f * (static_cast<float>(DISPLAY_SIZE_XY) / static_cast<float>(drawSideLength));
			//addChild( new ModuleDisplay(Vec(100, 100), pixelSize, true));

			//float brightness;

			int antCell = module->iFromXY(module->systemState->antX, module->systemState->antY);
			int shadowAntCell = module->iFromXY(module->systemState->shadowAntX, module->systemState->shadowAntY);
			
			int numCells = drawSideLength*drawSideLength;

			if(module->params[MusicalAnt::AUNTYLANGBUTTON_PARAM].getValue() == 1.0) {
				for(int i=0;i<CELLS;i++){
					cells.at(i) = Logos::AL_logo_144x144[i];//false;
				}
			}
			else {
				cells = module->systemState->cells;
			}
			

			for(int i=0; i < numCells; i++){
				x = i%drawSideLength; //increment x up to COL length then loop
				if((x == 0)&&(i != 0)){ //increment y once x hits positive multiple of COL length
					y++;
				}
				
				//nvgFillColor(draw.vg, (module->systemState->cells[i] ? nvgRGBA(0,255,0,255) : nvgRGBA(255,0,0,255)));
				if(cells.at(i)){
					nvgFillColor(draw.vg, ((random::uniform() < 0.5) ? nvgRGBA(0,255,0,PIXEL_BRIGHTNESS) : nvgRGBA(0,255,0,PIXEL_BRIGHTNESS+5)));
					nvgBeginPath(draw.vg);
					nvgRect(draw.vg, static_cast<float>(x)*pixelSize, static_cast<float>(y)*pixelSize, pixelSize, pixelSize);
					nvgFill(draw.vg);
				}
				if(i == shadowAntCell){
					if(!module->params[MusicalAnt::SHADOW_ANT_ON_PARAM].getValue()){
						nvgFillColor(draw.vg, nvgRGBA(0,70,0,255));
						nvgBeginPath(draw.vg);
						nvgRect(draw.vg, static_cast<float>(x)*pixelSize, static_cast<float>(y)*pixelSize, pixelSize, pixelSize);
						nvgFill(draw.vg);
					}
				}
				if(i == antCell){
					nvgFillColor(draw.vg, nvgRGBA(20,255,50,255));
					nvgBeginPath(draw.vg);
					nvgRect(draw.vg, static_cast<float>(x)*pixelSize, static_cast<float>(y)*pixelSize, pixelSize, pixelSize);
					nvgFill(draw.vg);
				}
				//addChild( new ModuleDisplay(module, Vec((x+1)*(pixelSize + gapSize) + DISPLAY_OFFSET_X, (y+1)*(pixelSize + gapSize) + DISPLAY_OFFSET_Y), pixelSize, i));
				//addChild(createLight<SmallLight<GreenLight>>(Vec((x+1)*6 + DISPLAY_OFFSET_X, (y+1)*6 + DISPLAY_OFFSET_Y), module, MusicalAnt::GRID_LIGHTS + i));
				//addChild(createLight<TinyLight<GreenLight>>(Vec((x+1)*3 + DISPLAY_OFFSET_X, (y+1)*3 + DISPLAY_OFFSET_Y), module, MusicalAnt::GRID_LIGHTS + i));
			}

			// Monitor fuzz
			float fuzzPixelSize = 2.2;
			x = 0;
			y = 0;
			for(int i=0; i < 3025; i++){
				x = i%55; //increment x up to COL length then loop
				if((i%55 == 0)&&(i!=0)){ //increment y once x hits positive multiple of COL length
					y++;
				}
				nvgFillColor(draw.vg, ((random::uniform() < 0.5) ? nvgRGBA(0,0,0,0) : nvgRGBA(255,255,255,8)));
				nvgBeginPath(draw.vg);
				nvgRect(draw.vg, static_cast<float>(x)*fuzzPixelSize, static_cast<float>(y)*fuzzPixelSize, fuzzPixelSize, fuzzPixelSize);
				nvgFill(draw.vg);
			}

			// Draw screen reflection over display
			nvgFillColor(draw.vg, nvgRGBA(255,255,255,7));
			nvgBeginPath(draw.vg);
			nvgCircle(draw.vg, 68, 54, 60);
			nvgFill(draw.vg);

			nvgBeginPath(draw.vg);
			nvgCircle(draw.vg, 77, 48, 40);
			nvgFill(draw.vg);

			nvgBeginPath(draw.vg);
			nvgCircle(draw.vg, 82, 43, 20);
			nvgFill(draw.vg);

			nvgFillColor(draw.vg, nvgRGBA(255,255,255,5));
			nvgBeginPath(draw.vg);
			nvgCircle(draw.vg, 87, 40, 8);
			nvgFill(draw.vg);

			// LCD shine
			nvgFillColor(draw.vg, nvgRGBA(255,255,255,10));
			nvgBeginPath(draw.vg);
			nvgMoveTo(draw.vg, 105, 326.7);
			nvgLineTo(draw.vg, 135, 326.7);
			nvgLineTo(draw.vg, 125, 336);
			nvgLineTo(draw.vg, 95, 336);
			nvgClosePath(draw.vg);
			nvgFill(draw.vg);

			nvgFillColor(draw.vg, nvgRGBA(255,255,255,15));
			nvgBeginPath(draw.vg);
			nvgMoveTo(draw.vg, 110, 326.7);
			nvgLineTo(draw.vg, 130, 326.7);
			nvgLineTo(draw.vg, 120, 336);
			nvgLineTo(draw.vg, 100, 336);
			nvgClosePath(draw.vg);
			nvgFill(draw.vg);

			nvgFillColor(draw.vg, nvgRGBA(255,255,255,20));
			nvgBeginPath(draw.vg);
			nvgMoveTo(draw.vg, 115, 326.7);
			nvgLineTo(draw.vg, 125, 326.7);
			nvgLineTo(draw.vg, 115, 336);
			nvgLineTo(draw.vg, 105, 336);
			nvgClosePath(draw.vg);
			nvgFill(draw.vg);
		}
	}

};


struct MusicalAntWidget : ModuleWidget {


	MusicalAntWidget(MusicalAnt *module) {
		setModule(module);
		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/MusicalAnt.svg")));

		addChild(createWidget<ScrewBlack>(Vec(0, 0)));
		addChild(createWidget<ScrewBlack>(Vec(box.size.x - 15, 0)));
		addChild(createWidget<ScrewBlack>(Vec(0, 365)));
		addChild(createWidget<ScrewBlack>(Vec(box.size.x - 15, 365)));

		CenteredLabel* const dynamicLabel = new CenteredLabel;
		dynamicLabel->box.pos = Vec(75, 182.2);

		addParam(createParam<RoundBlackKnob>(Vec(143.9, 177), module, MusicalAnt::CLOCK_PARAM));

		// Ant X panel widgets
		//addParam(createParam<RoundSmallBlackKnobSnap>(Vec(52.4, 218.5), module, MusicalAnt::OCTAVE_KNOB_X_PARAM));
		RoundSmallBlackKnobSnap *octaveKnobX = dynamic_cast<RoundSmallBlackKnobSnap*>(createParam<RoundSmallBlackKnobSnap>(Vec(52.4, 218.5), module, MusicalAnt::OCTAVE_KNOB_X_PARAM));
		RoundSmallBlackKnobSnap *noteKnobX = dynamic_cast<RoundSmallBlackKnobSnap*>(createParam<RoundSmallBlackKnobSnap>(Vec(76.9, 218.5), module, MusicalAnt::NOTE_KNOB_X_PARAM));
		RoundSmallBlackKnobSnap *scaleKnobX = dynamic_cast<RoundSmallBlackKnobSnap*>(createParam<RoundSmallBlackKnobSnap>(Vec(101.4, 218.5), module, MusicalAnt::SCALE_KNOB_X_PARAM));
		addOutput(createOutput<PJ301MPort>(Vec(148.9, 218.5), module, MusicalAnt::VOCT_OUTPUT_X));
		// Invert X V/Oct output
		addParam(createParam<CKSS>(Vec(127.5, 219.75), module, MusicalAnt::VOCT_INVERT_X_PARAM));
		
		// Ant Y panel widgets
		RoundSmallBlackKnobSnap *octaveKnobY = dynamic_cast<RoundSmallBlackKnobSnap*>(createParam<RoundSmallBlackKnobSnap>(Vec(52.4, 254), module, MusicalAnt::OCTAVE_KNOB_Y_PARAM));
		RoundSmallBlackKnobSnap *noteKnobY = dynamic_cast<RoundSmallBlackKnobSnap*>(createParam<RoundSmallBlackKnobSnap>(Vec(76.9, 254), module, MusicalAnt::NOTE_KNOB_Y_PARAM));
		RoundSmallBlackKnobSnap *scaleKnobY = dynamic_cast<RoundSmallBlackKnobSnap*>(createParam<RoundSmallBlackKnobSnap>(Vec(101.4, 254), module, MusicalAnt::SCALE_KNOB_Y_PARAM));
		addOutput(createOutput<PJ301MPort>(Vec(148.9, 254), module, MusicalAnt::VOCT_OUTPUT_Y));
		// Invert Y V/Oct output
		addParam(createParam<CKSS>(Vec(127.5, 255.25), module, MusicalAnt::VOCT_INVERT_Y_PARAM));

		// Shadow Ant X panel widgets
		RoundSmallBlackKnobSnap *shadowOctaveKnobX = dynamic_cast<RoundSmallBlackKnobSnap*>(createParam<RoundSmallBlackKnobSnap>(Vec(70.9, 289.5), module, MusicalAnt::OCTAVE_KNOB_SHADOW_X_PARAM));
		RoundSmallBlackKnobSnap *shadowNoteKnobX = dynamic_cast<RoundSmallBlackKnobSnap*>(createParam<RoundSmallBlackKnobSnap>(Vec(95.4, 289.5), module, MusicalAnt::NOTE_KNOB_SHADOW_X_PARAM));
		RoundSmallBlackKnobSnap *shadowScaleKnobX = dynamic_cast<RoundSmallBlackKnobSnap*>(createParam<RoundSmallBlackKnobSnap>(Vec(119.9, 289.5), module, MusicalAnt::SCALE_KNOB_SHADOW_X_PARAM));
		addOutput(createOutput<PJ301MPort>(Vec(148.9, 289.5), module, MusicalAnt::VOCT_OUTPUT_SHADOW_X));

		// Shadow Ant Y panel widgets
		RoundSmallBlackKnobSnap *shadowOctaveKnobY = dynamic_cast<RoundSmallBlackKnobSnap*>(createParam<RoundSmallBlackKnobSnap>(Vec(70.9, 325), module, MusicalAnt::OCTAVE_KNOB_SHADOW_Y_PARAM));
		RoundSmallBlackKnobSnap *shadowNoteKnobY = dynamic_cast<RoundSmallBlackKnobSnap*>(createParam<RoundSmallBlackKnobSnap>(Vec(95.4, 325), module, MusicalAnt::NOTE_KNOB_SHADOW_Y_PARAM));
		RoundSmallBlackKnobSnap *shadowScaleKnobY = dynamic_cast<RoundSmallBlackKnobSnap*>(createParam<RoundSmallBlackKnobSnap>(Vec(119.9, 325), module, MusicalAnt::SCALE_KNOB_SHADOW_Y_PARAM));
		addOutput(createOutput<PJ301MPort>(Vec(148.9, 325), module, MusicalAnt::VOCT_OUTPUT_SHADOW_Y));

		

		addInput(createInput<PJ301MPort>(Vec(115.9, 181), module, MusicalAnt::EXT_CLOCK_INPUT));
		addOutput(createOutput<PJ301MPort>(Vec(23.9, 181), module, MusicalAnt::GATE_OUTPUT));

		addOutput(createOutput<PJ301MPort>(Vec(52.9, 181), module, MusicalAnt::GATE_OUTPUT_LEFT));
		addOutput(createOutput<PJ301MPort>(Vec(82.9, 181), module, MusicalAnt::GATE_OUTPUT_RIGHT));
		

		addChild(createLight<SmallLight<GreenLight>>(Vec(108.9, 170), module, MusicalAnt::BLINK_LIGHT));

		// Shadow Ant on switch
		addParam(createParam<CKSS>(Vec(50.80, 305), module, MusicalAnt::SHADOW_ANT_ON_PARAM));

		// Effect Knob 
		addParam(createParam<RoundBlackSnapKnob>(Vec(253.9, 250), module, MusicalAnt::EFFECT_KNOB_PARAM));

		// Loop Mode Switch
		addParam(createParam<CKSS_Horizontal>(Vec(25, 290), module, MusicalAnt::LOOPMODE_SWITCH_PARAM));

		// Loop length knob

		//addParam(createParam<RoundSmallBlackKnobSnap>(Vec(23.65, 306.5), module, MusicalAnt::LOOP_LENGTH_PARAM));
		RoundSmallBlackKnobSnap *loopLengthKnob = dynamic_cast<RoundSmallBlackKnobSnap*>(createParam<RoundSmallBlackKnobSnap>(Vec(23.65, 306.5), module, MusicalAnt::LOOP_LENGTH_PARAM));
		// Loop length text

		//addChild( new LoopLengthTextLabel(module, Vec(253.55, 80), 20, 1, nvgRGBA(255,0,0,255) ) );

		// Resolution/SideLength Knob
		RoundSmallBlackKnobSnap *sideLengthKnob = dynamic_cast<RoundSmallBlackKnobSnap*>(createParam<RoundSmallBlackKnobSnap>(Vec(23.9, 218.5), module, MusicalAnt::SIDE_LENGTH_PARAM));

		// Skip Knob
		RoundSmallBlackKnobSnap *skipParamKnob = dynamic_cast<RoundSmallBlackKnobSnap*>(createParam<RoundSmallBlackKnobSnap>(Vec(23.9, 254), module, MusicalAnt::SKIP_PARAM));

		// AuntyLangButton!
		addParam(createParam<AuntyLangButton>(Vec(83.5, 352), module, MusicalAnt::AUNTYLANGBUTTON_PARAM));

		// Poly V/Oct out
		addOutput(createOutput<PJ301MPort>(Vec(23, 341.25), module, MusicalAnt::VOCT_OUTPUT_POLY_OUTPUT));
		
		
		octaveKnobX->connectLabel(dynamicLabel, module);
		noteKnobX->connectLabel(dynamicLabel, module);
		scaleKnobX->connectLabel(dynamicLabel, module);

		octaveKnobY->connectLabel(dynamicLabel, module);
		noteKnobY->connectLabel(dynamicLabel, module);
		scaleKnobY->connectLabel(dynamicLabel, module);

		shadowOctaveKnobX->connectLabel(dynamicLabel, module);
		shadowNoteKnobX->connectLabel(dynamicLabel, module);
		shadowScaleKnobX->connectLabel(dynamicLabel, module);

		shadowOctaveKnobY->connectLabel(dynamicLabel, module);
		shadowNoteKnobY->connectLabel(dynamicLabel, module);
		shadowScaleKnobY->connectLabel(dynamicLabel, module);

		loopLengthKnob->connectLabel(dynamicLabel, module);
		sideLengthKnob->connectLabel(dynamicLabel, module);
		skipParamKnob->connectLabel(dynamicLabel, module);

		addChild(dynamicLabel);

		addParam(octaveKnobX);
		addParam(noteKnobX);
		addParam(scaleKnobX);

		addParam(octaveKnobY);
		addParam(noteKnobY);
		addParam(scaleKnobY);

		addParam(shadowOctaveKnobX);
		addParam(shadowNoteKnobX);
		addParam(shadowScaleKnobX);

		addParam(shadowOctaveKnobY);
		addParam(shadowNoteKnobY);
		addParam(shadowScaleKnobY);

		addParam(loopLengthKnob);
		addParam(sideLengthKnob);
		addParam(skipParamKnob);

		// Create display
		ModuleDisplay *display = new ModuleDisplay();
		display->module = module;
		display->box.pos = Vec(DISPLAY_OFFSET_X, DISPLAY_OFFSET_Y);
		display->box.size = Vec(DISPLAY_SIZE_XY, DISPLAY_SIZE_XY);
		addChild(display);

	}
};


// Specify the Module and ModuleWidget subclass, human-readable
// author name for categorization per pluginInstance, module slug (should never
// change), human-readable module name, and any number of tags
// (found in `include/tags.hpp`) separated by commas.
// TODO Fix up tag
Model *modelMusicalAnt = createModel<MusicalAnt, MusicalAntWidget>("MusicalAnt");
