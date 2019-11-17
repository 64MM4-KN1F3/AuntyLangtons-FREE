#include "AuntyLangton.hpp"
#include <iostream>
#include <vector>
#include <algorithm>
#include <math.h>
using namespace std;
#define CELLS 20736
#define HISTORY_AMOUNT 100
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

// TODO 1 - 4 way switch: Off, Sparkle, Fade, Mirror - Off - Ant is normal, sparkle is random additive noise, fade is random subtractive noise, mirror is second ant with mirrored left/right moves. Knob controls amount of noise and fading amount on Fade. Fade utilises a second shadowAnt delayed by a power of 10 steps. Second ant has exact same function as normal ant, thereby undoing it's old moves.
// TODO 2 - uncomment and finish the manual drawing functionality
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


*/

struct MusicalAnt : Module, QuantizeUtils {
	enum ParamIds {
		PITCH_PARAM,
		RUN_PARAM,
		CLOCK_PARAM,
		RND_MODE_KNOB_PARAM,
		RND_TRIG_BTN_PARAM,
		RND_AMT_KNOB_PARAM,
		SCALE_KNOB_PARAM_X,
		SCALE_KNOB_PARAM_Y,
		SCALE_KNOB_PARAM_SHADOW_X,
		SCALE_KNOB_PARAM_SHADOW_Y,
		NOTE_KNOB_PARAM_X,
		NOTE_KNOB_PARAM_Y,
		NOTE_KNOB_PARAM_SHADOW_X,
		NOTE_KNOB_PARAM_SHADOW_Y,
		OCTAVE_KNOB_PARAM_X,
		OCTAVE_KNOB_PARAM_Y,
		OCTAVE_KNOB_PARAM_SHADOW_X,
		OCTAVE_KNOB_PARAM_SHADOW_Y,
		INDEX_PARAM,
		SKIP_PARAM,
		EFFECT_KNOB_PARAM,
		SHADOW_ANT_ON,
		SIDE_LENGTH_PARAM,
		STEP_FWD_BTN_PARAM,
		STEP_BCK_BTN_PARAM,
		LOOPMODE_SWITCH_PARAM,
		LOOP_LENGTH,
		VOCT_INVERT_X,
		VOCT_INVERT_Y,
		AUNTYLANGBUTTON_PARAM,
		VOCT_OUTPUT_POLY,
		NUM_PARAMS
	};
	enum InputIds {
		CLOCK_INPUT,
		EXT_CLOCK_INPUT,
		PITCH_INPUT,
		RND_TRIG_INPUT,
		RND_AMT_INPUT,
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
		NUM_OUTPUTS
	};
	enum LightIds {
		BLINK_LIGHT,
		ENUMS(GRID_LIGHTS, CELLS),
		NUM_LIGHTS
	};
	enum RndMode {
		RND_BASIC,
		RND_EUCLID,
		RND_SIN_WAVE,
		RND_LIFE_GLIDERS,
		NUM_RND_MODES
	};

	float phase = 0.0;
	float blinkPhase = 0.0;
	int index = 0;
	bool loopOn = false;
	int loopLength = 0;
	int loopIndex;
	dsp::SchmittTrigger clockTrigger;
	int fibo[7] = {8, 13, 21, 34, 55, 89, 144}; // short for Fibonacci (cause I forgot that's why I named it that).
	int sideLength;
	bool currentArrowOfTimeForward = true;
	bool lastArrowOfTimeForward = true;

	// New system representation.

	MusicalSystemState* systemState;
	MusicalInstruction* antBehaviour;


	MusicalAnt() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
		configParam(MusicalAnt::CLOCK_PARAM, -2.0f, 8.0f, 1.0f, "");
		configParam(MusicalAnt::OCTAVE_KNOB_PARAM_X, 0.0, 7.0, 2.0, "");
		configParam(MusicalAnt::NOTE_KNOB_PARAM_X, 0.0, QuantizeUtils::NUM_NOTES-1, QuantizeUtils::NOTE_C, "");
		configParam(MusicalAnt::SCALE_KNOB_PARAM_X, 0.0, QuantizeUtils::NUM_SCALES-1, QuantizeUtils::MINOR, "");
		configParam(MusicalAnt::VOCT_INVERT_X, 0.0f, 1.0f, 1.0f, "");
		configParam(MusicalAnt::OCTAVE_KNOB_PARAM_Y, 0.0, 7.0, 2.0, "");
		configParam(MusicalAnt::NOTE_KNOB_PARAM_Y, 0.0, QuantizeUtils::NUM_NOTES-1, QuantizeUtils::NOTE_C, "");
		configParam(MusicalAnt::SCALE_KNOB_PARAM_Y, 0.0, QuantizeUtils::NUM_SCALES-1, QuantizeUtils::MINOR, "");
		configParam(MusicalAnt::VOCT_INVERT_Y, 0.0f, 1.0f, 1.0f, "");
		configParam(MusicalAnt::OCTAVE_KNOB_PARAM_SHADOW_X, 0.0, 7.0, 2.0, "");
		configParam(MusicalAnt::NOTE_KNOB_PARAM_SHADOW_X, 0.0, QuantizeUtils::NUM_NOTES-1, QuantizeUtils::NOTE_C, "");
		configParam(MusicalAnt::SCALE_KNOB_PARAM_SHADOW_X, 0.0, QuantizeUtils::NUM_SCALES-1, QuantizeUtils::MINOR, "");
		configParam(MusicalAnt::OCTAVE_KNOB_PARAM_SHADOW_Y, 0.0, 7.0, 2.0, "");
		configParam(MusicalAnt::NOTE_KNOB_PARAM_SHADOW_Y, 0.0, QuantizeUtils::NUM_NOTES-1, QuantizeUtils::NOTE_C, "");
		configParam(MusicalAnt::SCALE_KNOB_PARAM_SHADOW_Y, 0.0, QuantizeUtils::NUM_SCALES-1, QuantizeUtils::MINOR, "");
		configParam(MusicalAnt::SHADOW_ANT_ON, 0.0f, 1.0f, 1.0f, "");
		configParam(MusicalAnt::EFFECT_KNOB_PARAM, 0.0f, 5.0f, 0.0, "");
		configParam(MusicalAnt::LOOPMODE_SWITCH_PARAM, 0.0f, 1.0f, 0.0f, "");
		configParam(MusicalAnt::LOOP_LENGTH, 0.0f, 31.0f, 0.0, "");
		configParam(MusicalAnt::SIDE_LENGTH_PARAM, 0.0f, 6.0f, INITIAL_RESOLUTION_KNOB_POSITION, "");
		configParam(MusicalAnt::SKIP_PARAM, 0.0f, 9.0f, 0.0f, "");

		sideLength = fibo[INITIAL_RESOLUTION_KNOB_POSITION];

		systemState = new MusicalSystemState();
		antBehaviour = new MusicalInstruction();

		onReset();
	}

	~MusicalAnt() {
		//systemState->cells.clear();
	}

	void onReset() {
		
		clearCells();
		setAntPosition(sideLength/2, sideLength/2, 0);
		
		setShadowAntPosition(sideLength/2, sideLength/2, 0);
		
		index = 0;
		loopIndex = 0;
		


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

		systemState->antX = sideLength/2;
		systemState->antY = sideLength/2;
		systemState->antDirectionDegrees = 0;

		systemState->shadowAntX = sideLength/2;
		systemState->shadowAntY = sideLength/2;
		systemState->shadowAntDirectionDegrees = 0;
		
	}

	json_t *dataToJson() override {
		json_t *rootJ = json_object();

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

		json_t *cellsJ = json_array();
		for (int i = 0; i < CELLS; i++) {
			json_t *cellJ = json_integer((int) systemState->cells.at(i));
			json_array_append_new(cellsJ, cellJ);
		}
		json_object_set_new(rootJ, "cells", cellsJ);

		/*
		ToJson TODO
		cellsHistory.clear();
		index settings ?
		*/
		
		return rootJ;
	}

	void dataFromJson(json_t *rootJ) override {

		json_t *cellsJ = json_object_get(rootJ, "cells");
		if (cellsJ) {
			for (int i = 0; i < CELLS; i++) {
				json_t *cellJ = json_array_get(cellsJ, i);
				if (cellJ)
					systemState->cells.at(i) = (bool) json_integer_value(cellJ);
			}
		}
	}

	int wrap(int kX, int const kLowerBound, int const kUpperBound) {
    	int range_size = kUpperBound - kLowerBound + 1;
	    if (kX < kLowerBound)
	    	kX += range_size * ((kLowerBound - kX) / range_size + 1);

    	return kLowerBound + (kX - kLowerBound) % range_size;
    }

	void setIndex(int index) {
		
		this->index = index;
	}

	bool getLoopOn() {
		return this->loopOn;
	}

	void setLoopOn(bool value) {
		this->loopOn = value;
	}

	int getLoopLength() {
		return this->loopLength;
	}

	void setLoopLength(int value) {
		this->loopLength = value;
	}

	int getIndex() {
		return this->index;
	}

	void setSideLength(int x){
		sideLength = fibo[x];
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

	void process(const ProcessArgs &args) override;

	void clearCells() {
		for(unsigned int i=0;i<CELLS;i++){
			systemState->cells.at(i) = false;
		}
	}

	float closestVoltageForX(int cellFromBottom){
		int octave = params[OCTAVE_KNOB_PARAM_X].getValue();
		int rootNote = params[NOTE_KNOB_PARAM_X].getValue();
		int scale = params[SCALE_KNOB_PARAM_X].getValue();
		return closestVoltageInScale(octave + (cellFromBottom * 0.0833), rootNote, scale);
	}

	float closestVoltageForShadowX(int cellFromBottom){
		int octave = params[OCTAVE_KNOB_PARAM_SHADOW_X].getValue();
		int rootNote = params[NOTE_KNOB_PARAM_SHADOW_X].getValue();
		int scale = params[SCALE_KNOB_PARAM_SHADOW_X].getValue();
		return closestVoltageInScale(octave + (cellFromBottom * 0.0833), rootNote, scale);
	}

	float closestVoltageForY(int cellFromBottom){
		int octave = params[OCTAVE_KNOB_PARAM_Y].getValue();
		int rootNote = params[NOTE_KNOB_PARAM_Y].getValue();
		int scale = params[SCALE_KNOB_PARAM_Y].getValue();
		return closestVoltageInScale(octave + (cellFromBottom * 0.0833), rootNote, scale);
	}


	float closestVoltageForShadowY(int cellFromBottom){
		int octave = params[OCTAVE_KNOB_PARAM_SHADOW_Y].getValue();
		int rootNote = params[NOTE_KNOB_PARAM_SHADOW_Y].getValue();
		int scale = params[SCALE_KNOB_PARAM_SHADOW_Y].getValue();
		return closestVoltageInScale(octave + (cellFromBottom * 0.0833), rootNote, scale);
	}
 
	void setCellOn(int cellX, int cellY, bool on){
		if(cellX >= 0 && cellX < sideLength && 
		   cellY >=0 && cellY < sideLength){
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
		float pixelSize = 0.9f * ((float) DISPLAY_SIZE_XY / (float) sideLength);
		setCellOn(int(displayX / pixelSize), int(displayY / pixelSize), on);
	}

	bool getCellStateByDisplayPos(float displayX, float displayY){
		float pixelSize = 0.9f * ((float) DISPLAY_SIZE_XY / (float) sideLength);
		return getCellState(int(displayX / pixelSize), int(displayY / pixelSize));
	}

	int iFromXY(int cellX, int cellY){
		return cellX + cellY * sideLength;
	}

	int xFromI(int cellI){
		return cellI % sideLength;
	}

	int yFromI(int cellI){
		return cellI / sideLength;
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
		// Ant

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

		bool shadowAntOn = (bool) !params[SHADOW_ANT_ON].getValue();

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

		int tempSideLength = (int) params[SIDE_LENGTH_PARAM].getValue();
		outputs[VOCT_OUTPUT_X].setVoltage(!params[VOCT_INVERT_X].getValue() ? closestVoltageForX(tempSideLength - getAntX()) : closestVoltageForX(getAntX()));
		outputs[VOCT_OUTPUT_Y].setVoltage(!params[VOCT_INVERT_Y].getValue() ? closestVoltageForY(tempSideLength - getAntY()) : closestVoltageForY(getAntY()));
		
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
		setIndex(index + steps);
	}

	// For more advanced Module features, read Rack's engine.hpp header file
	// - dataToJson, dataFromJson: serialization of internal data
	// - onSampleRateChange: event triggered by a change of sample rate
	// - onReset, onRandomize, onCreate, onDelete: implements special behavior when user clicks these from the context menu
};


void MusicalAnt::process(const ProcessArgs &args) {


	bool loopIsOn = params[LOOPMODE_SWITCH_PARAM].getValue();

	int currentIndex = getIndex();

	bool gateIn = false;
	int numberSteps = (int) params[SKIP_PARAM].getValue() + 1;

	

	loopLength = (params[LOOP_LENGTH].getValue() + 1);
	setLoopLength(loopLength);



	// Looping implementation
	

	if (inputs[EXT_CLOCK_INPUT].isConnected()) {
		// External clock
		if (clockTrigger.process(rescale(inputs[EXT_CLOCK_INPUT].getVoltage(), 0.1f, 2.f, 0.f, 1.f))) {

			if(getLoopOn() != loopIsOn) {
				loopIndex = currentIndex;
				walkAnt(-1*loopLength*numberSteps);
				setLoopOn(loopIsOn);
			}
			else if(loopIsOn && 
				(currentIndex >= loopIndex + numberSteps*loopLength)) {
				walkAnt(-1*numberSteps*loopLength);
			}
			else {
				walkAnt(numberSteps);
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
			if(getLoopOn() != loopIsOn) {
				loopIndex = currentIndex;
				walkAnt(-1*loopLength*numberSteps);
				setLoopOn(loopIsOn);
			}
			else if(loopIsOn && 
				(currentIndex >= loopIndex + numberSteps*loopLength)) {
				walkAnt(-1*numberSteps*loopLength);
			}
			else {
				walkAnt(numberSteps);
			} 
		}

		gateIn = (phase < 0.5f);
	}
	



	// TODO Fix up this var below. May not be needed, or at least needs refactoring
	int tempSideLength = (int) params[SIDE_LENGTH_PARAM].getValue();

	// Compute the frequency from the pitch parameter and input
	float pitch = params[PITCH_PARAM].getValue();
	setSideLength(tempSideLength);
	float gateOut = (gateIn ? 10.0f : 0.0f);
	pitch += inputs[PITCH_INPUT].getVoltage();
	pitch = clamp(pitch, -4.0f, 4.0f);



	params[INDEX_PARAM].setValue(currentIndex);


	outputs[GATE_OUTPUT].setVoltage(gateOut);

	outputs[VOCT_OUTPUT_POLY].setVoltage(!params[VOCT_INVERT_X].getValue() ? closestVoltageForX(tempSideLength - getAntX()) : closestVoltageForX(getAntX()), 0);;
	outputs[VOCT_OUTPUT_POLY].setVoltage(!params[VOCT_INVERT_Y].getValue() ? closestVoltageForY(tempSideLength - getAntY()) : closestVoltageForY(getAntY()), 1);;
	if((bool) !params[SHADOW_ANT_ON].getValue()) {
		outputs[VOCT_OUTPUT_POLY].setVoltage(closestVoltageForShadowX(getShadowAntX()), 2);
		outputs[VOCT_OUTPUT_POLY].setVoltage(closestVoltageForShadowY(getShadowAntY()), 3);
		// Don't forget to set the number of output channels
		outputs[VOCT_OUTPUT_POLY].setChannels(4);
	}
	else {
		outputs[VOCT_OUTPUT_POLY].setChannels(2);
	}

	//lights[BLINK_LIGHT].value = gateIn ? 1.0f : 0.0f;

}

struct ModuleDisplay : Widget {
	MusicalAnt *module;
	bool currentlyTurningOn = false;
	float initX = 0;
	float initY = 0;
	float dragX = 0;
	float dragY = 0;

	ModuleDisplay(){}


	void onButton(const event::Button &e) override {
		if (e.action == GLFW_PRESS && e.button == GLFW_MOUSE_BUTTON_LEFT) {
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
	
	void onDragStart(const event::DragStart &e) override {
		dragX = APP->scene->rack->mousePos.x;
		dragY = APP->scene->rack->mousePos.y;
	}

	void onDragMove(const event::DragMove &e) override {
		float newDragX = APP->scene->rack->mousePos.x;
		float newDragY = APP->scene->rack->mousePos.y;
		module->setCellOnByDisplayPos(initX+(newDragX-dragX), initY+(newDragY-dragY), currentlyTurningOn);
	}

	void draw(const DrawArgs &args) override {

		int x = 0;
		int y = 0;
		std::vector<bool> cells;
		if(module == NULL) return;

		float pixelSize = 0.9f * ((float) DISPLAY_SIZE_XY / (float) module->sideLength);
		//addChild( new ModuleDisplay(Vec(100, 100), pixelSize, true));

		//float brightness;

		int antCell = module->iFromXY(module->systemState->antX, module->systemState->antY);
		int shadowAntCell = module->iFromXY(module->systemState->shadowAntX, module->systemState->shadowAntY);
		
		int numCells = module->sideLength*module->sideLength;
		cells = module->systemState->cells;
		for(int i=0; i < numCells; i++){
			x = (int) i%module->sideLength; //increment x up to COL length then loop
			if((x == 0)&&(i != 0)){ //increment y once x hits positive multiple of COL length
				y++;
			}
			
			//nvgFillColor(vg, (module->systemState->cells[i] ? nvgRGBA(0,255,0,255) : nvgRGBA(255,0,0,255)));
			if(cells.at(i)){
				nvgFillColor(args.vg, ((random::uniform() < 0.5) ? nvgRGBA(0,255,0,PIXEL_BRIGHTNESS) : nvgRGBA(0,255,0,PIXEL_BRIGHTNESS+5)));
				nvgBeginPath(args.vg);
				nvgRect(args.vg, (float) x*pixelSize, (float) y*pixelSize, pixelSize, pixelSize);
				nvgFill(args.vg);
			}
			if(i == shadowAntCell){
				if(!module->params[MusicalAnt::SHADOW_ANT_ON].getValue()){
					nvgFillColor(args.vg, nvgRGBA(0,70,0,255));
					nvgBeginPath(args.vg);
					nvgRect(args.vg, (float) x*pixelSize, (float) y*pixelSize, pixelSize, pixelSize);
					nvgFill(args.vg);
				}
			}
			if(i == antCell){
				nvgFillColor(args.vg, nvgRGBA(20,255,50,255));
				nvgBeginPath(args.vg);
				nvgRect(args.vg, (float) x*pixelSize, (float) y*pixelSize, pixelSize, pixelSize);
				nvgFill(args.vg);
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
			nvgFillColor(args.vg, ((random::uniform() < 0.5) ? nvgRGBA(0,0,0,0) : nvgRGBA(255,255,255,8)));
			nvgBeginPath(args.vg);
			nvgRect(args.vg, (float) x*fuzzPixelSize, (float) y*fuzzPixelSize, fuzzPixelSize, fuzzPixelSize);
			nvgFill(args.vg);
		}

		// Draw screen reflection over display
		nvgFillColor(args.vg, nvgRGBA(255,255,255,7));
		nvgBeginPath(args.vg);
		//nvgRect(args.vg, (float) x*pixelSize, (float) y*pixelSize, pixelSize, pixelSize);
		nvgCircle(args.vg, 68, 54, 60);
		nvgFill(args.vg);

		nvgBeginPath(args.vg);
		//nvgRect(args.vg, (float) x*pixelSize, (float) y*pixelSize, pixelSize, pixelSize);
		nvgCircle(args.vg, 77, 48, 40);
		nvgFill(args.vg);

		nvgBeginPath(args.vg);
		//nvgRect(args.vg, (float) x*pixelSize, (float) y*pixelSize, pixelSize, pixelSize);
		nvgCircle(args.vg, 82, 43, 20);
		nvgFill(args.vg);

		nvgFillColor(args.vg, nvgRGBA(255,255,255,5));
		nvgBeginPath(args.vg);
		//nvgRect(args.vg, (float) x*pixelSize, (float) y*pixelSize, pixelSize, pixelSize);
		nvgCircle(args.vg, 87, 40, 8);
		nvgFill(args.vg);

		//nvgFillColor(args.vg, nvgRGBA(0,0,0,0));

		// LCD shine
		/*nvgFillColor(args.vg, nvgRGBA(255,255,255,10));
		nvgBeginPath(args.vg);
		nvgMoveTo(args.vg, 105, 326.7);
		nvgLineTo(args.vg, 135, 326.7);
		nvgLineTo(args.vg, 125, 336);
		nvgLineTo(args.vg, 95, 336);
		nvgClosePath(args.vg);
		nvgFill(args.vg);

		nvgFillColor(args.vg, nvgRGBA(255,255,255,15));
		nvgBeginPath(args.vg);
		nvgMoveTo(args.vg, 110, 326.7);
		nvgLineTo(args.vg, 130, 326.7);
		nvgLineTo(args.vg, 120, 336);
		nvgLineTo(args.vg, 100, 336);
		nvgClosePath(args.vg);
		nvgFill(args.vg);

		nvgFillColor(args.vg, nvgRGBA(255,255,255,20));
		nvgBeginPath(args.vg);
		nvgMoveTo(args.vg, 115, 326.7);
		nvgLineTo(args.vg, 125, 326.7);
		nvgLineTo(args.vg, 115, 336);
		nvgLineTo(args.vg, 105, 336);
		nvgClosePath(args.vg);
		nvgFill(args.vg);*/

	}

};


struct MusicalAntWidget : ModuleWidget {
	MusicalAnt *module;


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
		//addParam(createParam<RoundSmallBlackKnobSnap>(Vec(52.4, 218.5), module, MusicalAnt::OCTAVE_KNOB_PARAM_X));
		RoundSmallBlackKnobSnap *octaveKnobX = dynamic_cast<RoundSmallBlackKnobSnap*>(createParam<RoundSmallBlackKnobSnap>(Vec(52.4, 218.5), module, MusicalAnt::OCTAVE_KNOB_PARAM_X));
		RoundSmallBlackKnobSnap *noteKnobX = dynamic_cast<RoundSmallBlackKnobSnap*>(createParam<RoundSmallBlackKnobSnap>(Vec(76.9, 218.5), module, MusicalAnt::NOTE_KNOB_PARAM_X));
		RoundSmallBlackKnobSnap *scaleKnobX = dynamic_cast<RoundSmallBlackKnobSnap*>(createParam<RoundSmallBlackKnobSnap>(Vec(101.4, 218.5), module, MusicalAnt::SCALE_KNOB_PARAM_X));
		addOutput(createOutput<PJ301MPort>(Vec(148.9, 218.5), module, MusicalAnt::VOCT_OUTPUT_X));
		// Invert X V/Oct output
		addParam(createParam<CKSS>(Vec(127.5, 219.75), module, MusicalAnt::VOCT_INVERT_X));
		
		// Ant Y panel widgets
		RoundSmallBlackKnobSnap *octaveKnobY = dynamic_cast<RoundSmallBlackKnobSnap*>(createParam<RoundSmallBlackKnobSnap>(Vec(52.4, 254), module, MusicalAnt::OCTAVE_KNOB_PARAM_Y));
		RoundSmallBlackKnobSnap *noteKnobY = dynamic_cast<RoundSmallBlackKnobSnap*>(createParam<RoundSmallBlackKnobSnap>(Vec(76.9, 254), module, MusicalAnt::NOTE_KNOB_PARAM_Y));
		RoundSmallBlackKnobSnap *scaleKnobY = dynamic_cast<RoundSmallBlackKnobSnap*>(createParam<RoundSmallBlackKnobSnap>(Vec(101.4, 254), module, MusicalAnt::SCALE_KNOB_PARAM_Y));
		addOutput(createOutput<PJ301MPort>(Vec(148.9, 254), module, MusicalAnt::VOCT_OUTPUT_Y));
		// Invert Y V/Oct output
		addParam(createParam<CKSS>(Vec(127.5, 255.25), module, MusicalAnt::VOCT_INVERT_Y));

		// Shadow Ant X panel widgets
		RoundSmallBlackKnobSnap *shadowOctaveKnobX = dynamic_cast<RoundSmallBlackKnobSnap*>(createParam<RoundSmallBlackKnobSnap>(Vec(70.9, 289.5), module, MusicalAnt::OCTAVE_KNOB_PARAM_SHADOW_X));
		RoundSmallBlackKnobSnap *shadowNoteKnobX = dynamic_cast<RoundSmallBlackKnobSnap*>(createParam<RoundSmallBlackKnobSnap>(Vec(95.4, 289.5), module, MusicalAnt::NOTE_KNOB_PARAM_SHADOW_X));
		RoundSmallBlackKnobSnap *shadowScaleKnobX = dynamic_cast<RoundSmallBlackKnobSnap*>(createParam<RoundSmallBlackKnobSnap>(Vec(119.9, 289.5), module, MusicalAnt::SCALE_KNOB_PARAM_SHADOW_X));
		addOutput(createOutput<PJ301MPort>(Vec(148.9, 289.5), module, MusicalAnt::VOCT_OUTPUT_SHADOW_X));

		// Shadow Ant Y panel widgets
		RoundSmallBlackKnobSnap *shadowOctaveKnobY = dynamic_cast<RoundSmallBlackKnobSnap*>(createParam<RoundSmallBlackKnobSnap>(Vec(70.9, 325), module, MusicalAnt::OCTAVE_KNOB_PARAM_SHADOW_Y));
		RoundSmallBlackKnobSnap *shadowNoteKnobY = dynamic_cast<RoundSmallBlackKnobSnap*>(createParam<RoundSmallBlackKnobSnap>(Vec(95.4, 325), module, MusicalAnt::NOTE_KNOB_PARAM_SHADOW_Y));
		RoundSmallBlackKnobSnap *shadowScaleKnobY = dynamic_cast<RoundSmallBlackKnobSnap*>(createParam<RoundSmallBlackKnobSnap>(Vec(119.9, 325), module, MusicalAnt::SCALE_KNOB_PARAM_SHADOW_Y));
		addOutput(createOutput<PJ301MPort>(Vec(148.9, 325), module, MusicalAnt::VOCT_OUTPUT_SHADOW_Y));

		

		addInput(createInput<PJ301MPort>(Vec(115.9, 181), module, MusicalAnt::EXT_CLOCK_INPUT));
		addOutput(createOutput<PJ301MPort>(Vec(23.9, 181), module, MusicalAnt::GATE_OUTPUT));

		addOutput(createOutput<PJ301MPort>(Vec(52.9, 181), module, MusicalAnt::GATE_OUTPUT_LEFT));
		addOutput(createOutput<PJ301MPort>(Vec(82.9, 181), module, MusicalAnt::GATE_OUTPUT_RIGHT));
		

		addChild(createLight<SmallLight<GreenLight>>(Vec(108.9, 170), module, MusicalAnt::BLINK_LIGHT));

		// Shadow Ant on switch
		addParam(createParam<CKSS>(Vec(50.80, 305), module, MusicalAnt::SHADOW_ANT_ON));

		// Effect Knob 
		addParam(createParam<RoundBlackSnapKnob>(Vec(253.9, 250), module, MusicalAnt::EFFECT_KNOB_PARAM));

		// Loop Mode Switch
		addParam(createParam<CKSS_Horizontal>(Vec(25, 290), module, MusicalAnt::LOOPMODE_SWITCH_PARAM));

		// Loop length knob

		//addParam(createParam<RoundSmallBlackKnobSnap>(Vec(23.65, 306.5), module, MusicalAnt::LOOP_LENGTH));
		RoundSmallBlackKnobSnap *loopLengthKnob = dynamic_cast<RoundSmallBlackKnobSnap*>(createParam<RoundSmallBlackKnobSnap>(Vec(23.65, 306.5), module, MusicalAnt::LOOP_LENGTH));
		// Loop length text

		//addChild( new LoopLengthTextLabel(module, Vec(253.55, 80), 20, 1, nvgRGBA(255,0,0,255) ) );

		// Resolution/SideLength Knob
		RoundSmallBlackKnobSnap *sideLengthKnob = dynamic_cast<RoundSmallBlackKnobSnap*>(createParam<RoundSmallBlackKnobSnap>(Vec(23.9, 218.5), module, MusicalAnt::SIDE_LENGTH_PARAM));

		// Skip Knob
		RoundSmallBlackKnobSnap *skipParamKnob = dynamic_cast<RoundSmallBlackKnobSnap*>(createParam<RoundSmallBlackKnobSnap>(Vec(23.9, 254), module, MusicalAnt::SKIP_PARAM));

		// AuntyLangButton!
		addParam(createParam<AuntyLangButton>(Vec(83.5, 352), module, MusicalAnt::AUNTYLANGBUTTON_PARAM));

		// Poly V/Oct out
		addOutput(createOutput<PJ301MPort>(Vec(23, 341.25), module, MusicalAnt::VOCT_OUTPUT_POLY));
		
		if(module){
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
		
	}
};


// Specify the Module and ModuleWidget subclass, human-readable
// author name for categorization per pluginInstance, module slug (should never
// change), human-readable module name, and any number of tags
// (found in `include/tags.hpp`) separated by commas.
// TODO Fix up tag
Model *modelMusicalAnt = createModel<MusicalAnt, MusicalAntWidget>("MusicalAnt");
