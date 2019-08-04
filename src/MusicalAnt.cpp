#include "AuntyLangton.hpp"
#include <iostream>
#include <vector>
#include <math.h>
using namespace std;
#define CELLS 20736
#define HISTORY_AMOUNT 100
#define DISPLAY_OFFSET_X 35
#define DISPLAY_OFFSET_Y 28
#define DISPLAY_SIZE_XY 135
#define X_POSITION 0
#define Y_POSITION 1
#define DIRECTION 3
#define PIXEL_BRIGHTNESS 140

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
RACK_DIR=/Users/my/Documents/Rack-SDK make dist

Run Rack in debug mode (display std out to terminal):
/Applications/Rack_0.6.app/Contents/MacOS/Rack

TODO - Fix Bug resulting in Segmentation Fault when HISTORY_AMOUNT is exceded. Command line output example:
HistoryBufferUsage: 94 and leaving white
shadowAntOn= 1Shadow Ant position: X2 Y6 Direction180

HistoryBufferUsage: 95 and leaving black
shadowAntOn= 1Shadow Ant position: X2 Y5 Direction90

HistoryBufferUsage: 96 and leaving black
shadowAntOn= 1Shadow Ant position: X3 Y5 Direction180

HistoryBufferUsage: 97 and leaving white
shadowAntOn= 1Shadow Ant position: X3 Y4 Direction90

HistoryBufferUsage: 98 and leaving white
shadowAntOn= 1Shadow Ant position: X4 Y4 Direction180

HistoryBufferUsage: 99 and leaving black
shadowAntOn= 1Shadow Ant position: X4 Y5 Direction270

HistoryBufferUsage: 99 and leaving black
shadowAntOn= 1Shadow Ant position: X3 Y5 Direction0

Segmentation fault: 11

*/

struct MusicalAnt : Module, QuantizeUtils, Logos {
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
		GATE_OUTPUT_X,
		GATE_OUTPUT_Y,
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
	int sideLength = 8;
	int index = 0;
	int shadowIndex = 0;
	bool loopOn = false;
	int loopLength = 0;
	int loopIndex;
	bool running = true;
	dsp::SchmittTrigger clockTrigger;
	dsp::SchmittTrigger runningTrigger;
	dsp::SchmittTrigger resetTrigger;
	int* antVector = new int[3]; // Can probably just make this a std::vector?
	//int* antVectorHistory[HISTORY_AMOUNT];
	int** antVectorHistory = new int*[HISTORY_AMOUNT];
	int* shadowAntVector = new int[3];
	int** shadowAntVectorHistory = new int*[HISTORY_AMOUNT];
	int historyBufferUsage = 0;
	int fibo[7] = {8, 13, 21, 34, 55, 89, 144}; // short for Fibonacci (cause I forgot that's why I named it that).

	int lastAntX, lastAntY;

	bool* cells = new bool[CELLS];
	bool** cellsHistory = new bool*[HISTORY_AMOUNT];


	MusicalAnt() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
		configParam(MusicalAnt::CLOCK_PARAM, -2.0f, 8.0f, 1.0f, "");
		configParam(MusicalAnt::OCTAVE_KNOB_PARAM_X, 0.0, 7.0, 2.0, "");
		configParam(MusicalAnt::NOTE_KNOB_PARAM_X, 0.0, QuantizeUtils::NUM_NOTES-1, QuantizeUtils::NOTE_C, "");
		configParam(MusicalAnt::SCALE_KNOB_PARAM_X, 0.0, QuantizeUtils::NUM_SCALES-1, QuantizeUtils::MINOR, "");
		configParam(MusicalAnt::VOCT_INVERT_X, 0.0f, 1.0f, 1.0f, "");
		configParam(MusicalAnt::OCTAVE_KNOB_PARAM_Y, -5.0, 7.0, 2.0, "");
		configParam(MusicalAnt::NOTE_KNOB_PARAM_Y, 0.0, QuantizeUtils::NUM_NOTES-1, QuantizeUtils::NOTE_C, "");
		configParam(MusicalAnt::SCALE_KNOB_PARAM_Y, 0.0, QuantizeUtils::NUM_SCALES-1, QuantizeUtils::MINOR, "");
		configParam(MusicalAnt::VOCT_INVERT_Y, 0.0f, 1.0f, 1.0f, "");
		configParam(MusicalAnt::OCTAVE_KNOB_PARAM_SHADOW_X, -5.0, 7.0, 2.0, "");
		configParam(MusicalAnt::NOTE_KNOB_PARAM_SHADOW_X, 0.0, QuantizeUtils::NUM_NOTES-1, QuantizeUtils::NOTE_C, "");
		configParam(MusicalAnt::SCALE_KNOB_PARAM_SHADOW_X, 0.0, QuantizeUtils::NUM_SCALES-1, QuantizeUtils::MINOR, "");
		configParam(MusicalAnt::OCTAVE_KNOB_PARAM_SHADOW_Y, -5.0, 7.0, 2.0, "");
		configParam(MusicalAnt::NOTE_KNOB_PARAM_SHADOW_Y, 0.0, QuantizeUtils::NUM_NOTES-1, QuantizeUtils::NOTE_C, "");
		configParam(MusicalAnt::SCALE_KNOB_PARAM_SHADOW_Y, 0.0, QuantizeUtils::NUM_SCALES-1, QuantizeUtils::MINOR, "");
		configParam(MusicalAnt::SHADOW_ANT_ON, 0.0f, 1.0f, 1.0f, "");
		configParam(MusicalAnt::EFFECT_KNOB_PARAM, 0.0f, 5.0f, 0.0, "");
		configParam(MusicalAnt::LOOPMODE_SWITCH_PARAM, 0.0f, 1.0f, 0.0f, "");
		configParam(MusicalAnt::LOOP_LENGTH, 0.0f, 95.0f, 31.0, "");
		configParam(MusicalAnt::SIDE_LENGTH_PARAM, 0.0f, 6.0f, 4.0, "");
		configParam(MusicalAnt::SKIP_PARAM, 0.0f, 9.0f, 0.0f, "");
		reset();
	}

	~MusicalAnt() {
		delete [] cells;
		delete [] cellsHistory;
		delete [] antVector;
		delete [] *antVectorHistory;
		delete [] shadowAntVector;
		delete [] *shadowAntVectorHistory;
	}

	void reset() {
		clearCells();
		setAntPosition(sideLength/2, sideLength/2, 0);
		setShadowAntPosition(sideLength/2, sideLength/2, 0);
		lastAntX = 0;
		lastAntY = 0;
		index = 0;
		shadowIndex = 0;
		historyBufferUsage = 0;

		for (int x = 0; x < HISTORY_AMOUNT; x++) {
  			cellsHistory[x] = new bool[CELLS];
		}
		for (int x = 0; x < HISTORY_AMOUNT; x++) {
  			antVectorHistory[x] = new int[3];
		}
		for (int x = 0; x < HISTORY_AMOUNT; x++) {
  			shadowAntVectorHistory[x] = new int[3];
		}
		//cout << "reset(setAntPosition(" << SIDE_LENGTH/2 << "," << SIDE_LENGTH/2 << "," << 0 << "))\n";
	}

	json_t *dataToJson() override {
		json_t *rootJ = json_object();
		
		json_t *cellsJ = json_array();
		for (int i = 0; i < CELLS; i++) {
			json_t *cellJ = json_integer((int) cells[i]);
			json_array_append_new(cellsJ, cellJ);
		}
		json_object_set_new(rootJ, "cells", cellsJ);
		
		return rootJ;
	}

	void dataFromJson(json_t *rootJ) override {
		json_t *cellsJ = json_object_get(rootJ, "cells");
		if (cellsJ) {
			for (int i = 0; i < CELLS; i++) {
				json_t *cellJ = json_array_get(cellsJ, i);
				if (cellJ)
					cells[i] = json_integer_value(cellJ);
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
		//cout << "EffectKnob value: " << pow(10, (int) params[EFFECT_KNOB_PARAM].getValue());
		this->index = index;
		if (this->index >= std::numeric_limits<int>::max())
			this->index = 0;
		int shadowDepth = pow(10, (int) params[EFFECT_KNOB_PARAM].getValue());
		if (this->index > shadowDepth)
			this->shadowIndex += 1; //this->index - shadowDepth;
	}

	void setLoopLength(int length) {
		loopLength = length;
	}

	int getIndex() {
		return this->index;
	}

	int getShadowIndex() {
		return this->shadowIndex;
	}

	void setSideLength(int x){
		sideLength = fibo[x];
	}

	void setAntPosition(int x, int y, int direction) {
		//cout << "setAntPosition(" << x << "," << y << "," << direction << ")\n";
		if(lastAntX == 0) {
			if (getIndex() >= 2) {
				this->lastAntX = this->antVector[X_POSITION];
			}
		}
		else {
			this->lastAntX = this->antVector[X_POSITION];
		}

		if(lastAntY == 0) {
			if (getIndex() >= 2) {
				this->lastAntY = this->antVector[Y_POSITION];
			}
		}
		else {
			this->lastAntY = this->antVector[Y_POSITION];
		}

		this->antVector[X_POSITION] = wrap(x, 0, sideLength-1);
		this->antVector[Y_POSITION] = wrap(y, 0, sideLength-1);
		this->antVector[DIRECTION] = wrap(direction, 0, 359);
	}

	void setShadowAntPosition(int x, int y, int direction) {
		this->shadowAntVector[X_POSITION] = wrap(x, 0, sideLength-1);
		this->shadowAntVector[Y_POSITION] = wrap(y, 0, sideLength-1);
		this->shadowAntVector[DIRECTION] = wrap(direction, 0, 359);
	}


	int getAntX() {
		return this->antVector[X_POSITION];
	}

	int getAntY() {
		return this->antVector[Y_POSITION];
	}

	int getShadowAntX() {
		return this->shadowAntVector[X_POSITION];
	}

	int getShadowAntY() {
		return this->shadowAntVector[Y_POSITION];
	}

	int getLastAntX() {
		return this->lastAntX;
	}

	int getLastAntY() {
		return this->lastAntY;
	}

	int getAntDirection() {
		return this->antVector[DIRECTION];
	}

	void process(const ProcessArgs &args) override;

	void clearCells() {
		for(int i=0;i<CELLS;i++){
			cells[i] = Logos::AL_logo_144x144[i];//false;
		}
		// Testing Logo
		//std::copy(cells, cells+CELLS, Logos::AL_logo_144x144);
	}

	void randomizeCells() {
		//clearCells();
		float rndAmt = params[RND_AMT_KNOB_PARAM].getValue() + inputs[RND_AMT_INPUT].getVoltage()*0.1;
		switch(int(params[RND_MODE_KNOB_PARAM].getValue())){
			case RND_BASIC:{
				int numCells = sideLength*sideLength;
				for(int i=0;i<numCells;i++){
					setCellOn(xFromI(i), yFromI(i), random::uniform() < rndAmt);
				}
				break;
			}
			case RND_EUCLID:{
				for(int y=0; y < sideLength; y++){
					if(random::uniform() < rndAmt){
						int div = int(random::uniform() * sideLength * 0.5) + 1;
						for(int x=0; x < sideLength; x++){
							setCellOn(x, y, x % div == 0);
						}
					}
				}
				break;
			}
			case RND_SIN_WAVE:{
				int sinCount = int(rndAmt * 3) + 1;
				for(int i=0;i<sinCount;i++){
					float angle = 0;
					float angleInc = random::uniform();
					float offset = sideLength * 0.5;
					for(int x=0;x<sideLength;x+=1){
						int y = int(offset + (sinf(angle)*(offset)));
						setCellOn(x, y, true);
						angle+=angleInc;
					}
				}
				break;
			}
			case RND_LIFE_GLIDERS:{
				int gliderCount = int(rndAmt * 20);
				int size = 3;
				for(int i=0;i<gliderCount;i++){
					int x = size + int(random::uniform() * (sideLength-size*2));
					int y = size + int(random::uniform() * (sideLength-size*2));
					if(random::uniform() < 0.5){
						//down
						if(random::uniform() < 0.5){
							//right
							setCellOn(x, y, true);
							setCellOn(x+1, y+1, true);
							setCellOn(x+1, y+2, true);
							setCellOn(x, y+2, true);
							setCellOn(x-1, y+2, true);
						} else {
							//left
							setCellOn(x, y, true);
							setCellOn(x-1, y+1, true);
							setCellOn(x+1, y+2, true);
							setCellOn(x, y+2, true);
							setCellOn(x-1, y+2, true);
						}
					} else {
						//up
						if(random::uniform() < 0.5){
							//right
							setCellOn(x, y, true);
							setCellOn(x+1, y-1, true);
							setCellOn(x+1, y-2, true);
							setCellOn(x, y-2, true);
							setCellOn(x-1, y-2, true);
						} else {
							//left
							setCellOn(x, y, true);
							setCellOn(x-1, y-1, true);
							setCellOn(x+1, y-2, true);
							setCellOn(x, y-2, true);
							setCellOn(x-1, y-2, true);
						}
					}
				}
				break;
			}
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
			cells[iFromXY(cellX, cellY)] = on;
		}
	}

	bool isCellOn(int cellX, int cellY){
		return cells[iFromXY(cellX, cellY)];
	}

	void setCellOnByDisplayPos(float displayX, float displayY, bool on){
		float pixelSize = 0.9f * ((float) DISPLAY_SIZE_XY / (float) sideLength);
		setCellOn(int(displayX / pixelSize), int(displayY / pixelSize), on);
	}

	bool isCellOnByDisplayPos(float displayX, float displayY){
		float pixelSize = 0.9f * ((float) DISPLAY_SIZE_XY / (float) sideLength);
		return isCellOn(int(displayX / pixelSize), int(displayY / pixelSize));
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

	void stepAnt(){
		/*Pseudo:
		- current cell is set to opposite
		- case for 4 direction where setAntPosition is updated
		*/
		// Get current position value and store for later
		int currPositionX = getAntX();
		int currPositionY = getAntY();
		bool currPositionValue = isCellOn(currPositionX, currPositionY);
		int currDirection = getAntDirection();
		int newDirection;

		// Save last cell state
		int historyIndex = index % HISTORY_AMOUNT;
		//*cellsHistory[historyIndex] = new bool[CELLS];
		//cout << "\nCellHistoryIndex: " << historyIndex;
		// Record current cell state to historical snapshot
		historyBufferUsage = std::min(historyBufferUsage + 1, HISTORY_AMOUNT - 1);
		cout << "\nHistoryBufferUsage: " << historyBufferUsage;
		//historyBufferUsage = historyBufferUsage + 1;

		for(int t = 0; t < CELLS; t++){
				//cout << cellsHistory[20][t];
				cellsHistory[historyIndex][t] = cells[t];
		}


		// Record current ant vector to historical snapshot
		antVectorHistory[historyIndex][X_POSITION] = currPositionX;
		antVectorHistory[historyIndex][Y_POSITION] = currPositionY;
		antVectorHistory[historyIndex][DIRECTION] = currDirection;


		//Testing history (if index reaches 100 then reset cell state to 20th move)
		/*if (historyIndex >= 8) {
			//cells = cellsHistory[10];
			setAntPosition(antVectorHistory[0][X_POSITION], antVectorHistory[0][Y_POSITION], antVectorHistory[0][DIRECTION]);
			setShadowAntPosition(shadowAntVectorHistory[0][X_POSITION], shadowAntVectorHistory[0][Y_POSITION], shadowAntVectorHistory[0][DIRECTION]);
			for(int t = 0; t < CELLS; t++){
				//cout << cellsHistory[20][t];
				cells[t] = cellsHistory[0][t];
				
			}
			

			
			index = 0;
		}*/



		//cout << "Ant position: X" << currPositionX << " Y" << currPositionY << " Direction" << currDirection << "\n";

		newDirection = wrap(currDirection + (currPositionValue ? -90 : 90), 0, 359);

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
		// Previous cell position is set to opposite
		setCellOn(currPositionX, currPositionY, !currPositionValue);
		cout << " and leaving " << (!currPositionValue ? "black\n" : "white\n");
		bool shadowAntOn = (bool) !params[SHADOW_ANT_ON].getValue();
		cout << "shadowAntOn= " << std::to_string(shadowAntOn);

		// Mirror Ant
		if (shadowAntOn) {
			if (this->index > pow(10, (int) params[EFFECT_KNOB_PARAM].getValue())) {
				// Get current shadow ant position value and store for later
				int currShadowAntPositionX = this->shadowAntVector[X_POSITION];
				int currShadowAntPositionY = this->shadowAntVector[Y_POSITION];
				bool currShadowAntPositionValue = isCellOn(currShadowAntPositionX, currShadowAntPositionY);
				int currShadowAntDirection = this->shadowAntVector[DIRECTION];
				int newShadowAntDirection;

				shadowAntVectorHistory[historyIndex][X_POSITION] = currShadowAntPositionX;
				shadowAntVectorHistory[historyIndex][Y_POSITION] = currShadowAntPositionY;
				shadowAntVectorHistory[historyIndex][DIRECTION] = currShadowAntDirection;

				// Record current ant vector to historical snapshot

				/*shadowAntVectorHistory[historyIndex][X_POSITION] = currShadowAntPositionX;
				shadowAntVectorHistory[historyIndex][Y_POSITION] = currShadowAntPositionY;
				shadowAntVectorHistory[historyIndex][DIRECTION] = currShadowAntDirection;*/

				cout << "Shadow Ant position: X" << currShadowAntPositionX << " Y" << currShadowAntPositionY << " Direction" << currShadowAntDirection << "\n";
					
				newShadowAntDirection = wrap(currShadowAntDirection + (currShadowAntPositionValue ? -90 : 90), 0, 359);

					switch(newShadowAntDirection) {
						case 90 : {
							// Ant goes up
							setShadowAntPosition(currShadowAntPositionX, currShadowAntPositionY - 1, newShadowAntDirection);
							break;
						}
						case 180 : {
							// Ant goes right
							setShadowAntPosition(currShadowAntPositionX + 1, currShadowAntPositionY, newShadowAntDirection);
							break;
						}
						case 270 : {
							// Ant goes down
							setShadowAntPosition(currShadowAntPositionX, currShadowAntPositionY + 1, newShadowAntDirection);
							break;
						}
						case 0 : {
							// Ant goes left
							setShadowAntPosition(currShadowAntPositionX - 1, currShadowAntPositionY, newShadowAntDirection);
							break;
						}
					}
					setCellOn(currShadowAntPositionX, currShadowAntPositionY, !currShadowAntPositionValue);
			}

			// TODO - use the commented two lines below if adding VOCT invert switches to shadow ant X and Y
			//outputs[VOCT_OUTPUT_SHADOW_X].setVoltage(params[VOCT_INVERT_SHADOW_X].getValue() ? closestVoltageForShadowX(tempSideLength - getShadowAntX()) : closestVoltageForShadowX(getShadowAntX()));
			//outputs[VOCT_OUTPUT_SHADOW_Y].setVoltage(params[VOCT_INVERT_SHADOW_Y].getValue() ? closestVoltageForShadowY(tempSideLength - getShadowAntY()) : closestVoltageForShadowY(getShadowAntY()));
			outputs[VOCT_OUTPUT_SHADOW_X].setVoltage(closestVoltageForShadowX(getShadowAntX()));
			outputs[VOCT_OUTPUT_SHADOW_Y].setVoltage(closestVoltageForShadowY(getShadowAntY()));
		}
		// Outputting voltage based on Ant X and Y position.
		//outputs[VOCT_OUTPUT_X].setVoltage(powf(2.0f, (float)getAntX() / (float) sideLength));
		//outputs[VOCT_OUTPUT_Y].setVoltage(powf(2.0f, (float)getAntY() / (float) sideLength));
		int tempSideLength = (int) params[SIDE_LENGTH_PARAM].getValue();
		outputs[VOCT_OUTPUT_X].setVoltage(!params[VOCT_INVERT_X].getValue() ? closestVoltageForX(tempSideLength - getAntX()) : closestVoltageForX(getAntX()));
		outputs[VOCT_OUTPUT_Y].setVoltage(!params[VOCT_INVERT_Y].getValue() ? closestVoltageForY(tempSideLength - getAntY()) : closestVoltageForY(getAntY()));
		//TESTING
		cout << "\n@nt X VOCT OUTPUT: " << std::to_string(params[VOCT_INVERT_X].getValue() ? closestVoltageForX(tempSideLength - getAntX()) : closestVoltageForX(getAntX()));
		cout << "\n@nt Y VOCT OUTPUT: " << std::to_string(params[VOCT_INVERT_Y].getValue() ? closestVoltageForY(tempSideLength - getAntY()) : closestVoltageForY(getAntY()));

		if(index > 1) {
			int lastAntDirection = antVectorHistory[index - 1][DIRECTION];
			int currentAntDirection = getAntDirection();
			//cout << "\n###########################";
			//cout << "\n@#$@#$ LAST ANT DIR: " << to_string(lastAntDirection);
			//cout << "\n@#$@#$ CURR ANT DIR: " << to_string(currentAntDirection);
			//cout << "\n###########################";

			int leftTurn = wrap(currDirection - 90, 0, 359);
			int rightTurn = wrap(currDirection + 90, 0, 359);

			if(currentAntDirection != lastAntDirection) {
				if (currentAntDirection == leftTurn) {
					//Ant is turning left
					cout << "\nANT TURNING LEFT!";
					outputs[GATE_OUTPUT_X].setVoltage(outputs[GATE_OUTPUT_X].getVoltage() == 0.0f ? 10.0f : 0.0f);
				}
				else if (currentAntDirection == rightTurn) {
					//Ant is turning right
					cout << "\nANT TURNING RIGHT!";
					outputs[GATE_OUTPUT_Y].setVoltage(outputs[GATE_OUTPUT_Y].getVoltage() == 0.0f ? 10.0f : 0.0f);
				}
				else {
					//Ant is off the rails
					cout << "BIG PROBLEM!: Ant is of the rails!!";
				}
			}
		}

	}

	void walkAnt(int steps) {
		for(int i = 0; i < steps; i++) {
			setIndex(index + 1);
			stepAnt();
		}

	}

	void wayBackMachine(int stepsBack) {
		int currIndex = index;
		if(stepsBack > historyBufferUsage){
			stepsBack = historyBufferUsage;
		}
		if(index < stepsBack) {
			stepsBack = 0;
		}

		historyBufferUsage -= stepsBack;
		cout << "\nHistoryBufferUsage: " << historyBufferUsage;
		cout << "\nStepSkippingAmount: " << std::to_string((int) params[SKIP_PARAM].getValue()) << "\n";
		int historyTarget = (abs(currIndex - stepsBack)) % HISTORY_AMOUNT;
		setAntPosition(antVectorHistory[historyTarget][X_POSITION], antVectorHistory[historyTarget][Y_POSITION], antVectorHistory[historyTarget][DIRECTION]);
		setShadowAntPosition(shadowAntVectorHistory[historyTarget][X_POSITION], shadowAntVectorHistory[historyTarget][Y_POSITION], shadowAntVectorHistory[historyTarget][DIRECTION]);
		cout << "Writing cell history for target state: " << historyTarget;

		/*
		//TESTING
		// Printing out cell state to std out
		int numCells = sideLength*sideLength;
		cout << "---HISTORY-TARGET---";
		for(int i=0; i < numCells; i++){
			if(i%sideLength == 0){ //increment y once x hits positive multiple of COL length
				cout << "\n";
			}
			cout << cellsHistory[historyTarget][i];
		}

		cout << "---BEFORE-WAY-BACK---";
		for(int i=0; i < numCells; i++){
			if(i%sideLength == 0){ //increment y once x hits positive multiple of COL length
				cout << "\n";
			}
			cout << cells[i];
		}
		*/

		std::copy(cellsHistory[historyTarget], cellsHistory[historyTarget]+CELLS, cells);

		/*
		cout << "\n---AFTER-WAY-BACK---";
		for(int i=0; i < numCells; i++){
			if(i%sideLength == 0){ //increment y once x hits positive multiple of COL length
				cout << "\n";
			}
			cout << cells[i];
		}
		*/

		index -= stepsBack;
		if (index < 1){
			index = 1;
			lastAntX = 0;
			lastAntY = 0;
			cout << "GOES TO: IF";
		}
		else {
			cout << "GOES TO: ELSE";
			lastAntX = antVectorHistory[historyTarget - 1][X_POSITION];
			lastAntY = antVectorHistory[historyTarget - 1][Y_POSITION];
		}

	}

	// For more advanced Module features, read Rack's engine.hpp header file
	// - dataToJson, dataFromJson: serialization of internal data
	// - onSampleRateChange: event triggered by a change of sample rate
	// - onReset, onRandomize, onCreate, onDelete: implements special behavior when user clicks these from the context menu
};


void MusicalAnt::process(const ProcessArgs &args) {

	//Debug
	//cout << "Ant - X: " << getAntX() << " Y: " << getAntY() << " Direction: " << getAntDirection() << "\n";

	// Run
	if (runningTrigger.process(params[RUN_PARAM].getValue())) {
		running = !running;
	}

	bool gateIn = false;
	int numberSteps = (int) params[SKIP_PARAM].getValue() + 1;
	if((params[LOOPMODE_SWITCH_PARAM].getValue() != loopOn) & (params[LOOPMODE_SWITCH_PARAM].getValue() == true)) {
		// ^^ Loop switch has just been turned on.
		// Store current index value. This is the loop point.
		loopIndex = index;
		cout << "\nLoopIndex: " << loopIndex;
	}
	loopOn = params[LOOPMODE_SWITCH_PARAM].getValue();

	setLoopLength(params[LOOP_LENGTH].getValue() + 1);

	if (running) {
			if (inputs[EXT_CLOCK_INPUT].isConnected()) {
				// External clock
				if (clockTrigger.process(rescale(inputs[EXT_CLOCK_INPUT].getVoltage(), 0.1f, 2.f, 0.f, 1.f))) {
					walkAnt(numberSteps);
				}
				gateIn = clockTrigger.isHigh();
			}
			else {
				// Internal clock
				float clockTime = powf(2.0f, params[CLOCK_PARAM].getValue());
				phase += clockTime * args.sampleTime;
				if (phase >= 1.0f) {
					phase -= 1.0f;
					walkAnt(numberSteps);
				}

				gateIn = (phase < 0.5f);
			}


			/* Manually step the system forward by one
			if (clockTrigger.process(params[STEP_FWD_BTN_PARAM].getValue())) {
				walkAnt(1);
				params[STEP_FWD_BTN_PARAM].getValue() = 0.0;
			}

			// Manually step the system backward by one
			if (clockTrigger.process(params[STEP_BCK_BTN_PARAM].getValue())) {
				wayBackMachine(1);
				params[STEP_BCK_BTN_PARAM].getValue() = 0.0;
			}*/

			// Looping implementation
			if ((loopOn == true) && (loopLength != 0) && (loopLength < index) && (index > loopIndex)){
				//^^ Loop must not be default value of zero and must be less than index but equal or more than saved loopIndex
				wayBackMachine(loopLength);
			}

			
	}

	// TODO Fix up this var below. May not be needed, or at least needs refactoring
	int tempSideLength = (int) params[SIDE_LENGTH_PARAM].getValue();

	// Compute the frequency from the pitch parameter and input
	float pitch = params[PITCH_PARAM].getValue();
	setSideLength(tempSideLength);
	float gateOut = (gateIn ? 10.0f : 0.0f);
	pitch += inputs[PITCH_INPUT].getVoltage();
	pitch = clamp(pitch, -4.0f, 4.0f);

	

	params[INDEX_PARAM].setValue(getIndex());


	outputs[GATE_OUTPUT].setVoltage(gateOut);

	// Separate gate outputs are used for X and Y
	/*if (getAntX() != getLastAntX()) {
				outputs[GATE_OUTPUT_X].setVoltage(gateOut);
	}
	if (getAntY() != getLastAntY()) {
				outputs[GATE_OUTPUT_Y].setVoltage(gateOut);
	}*/
	


	lights[BLINK_LIGHT].value = gateIn ? 1.0f : 0.0f;
	//lights[GRID_LIGHT_ON].value = 1.0f;
	//lights[GRID_LIGHT_OFF].value = 0.0f;
	int numCells = sideLength*sideLength;
	for(int i = 0; i < numCells; i++) {
		lights[GRID_LIGHTS + i].value = (cells[i]);
	}
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
				currentlyTurningOn = !module->isCellOnByDisplayPos(initX, initY);
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

	void draw(NVGcontext *vg) override {

		int x = 0;
		int y = 0;
		if(module) {
			float pixelSize = 0.9f * ((float) DISPLAY_SIZE_XY / (float) module->sideLength);
			//addChild( new ModuleDisplay(Vec(100, 100), pixelSize, true));

			//float brightness;

			int shadowAntCell = module->iFromXY(module->shadowAntVector[X_POSITION], module->shadowAntVector[Y_POSITION]);
			
			int numCells = module->sideLength*module->sideLength;
			for(int i=0; i < numCells; i++){
				x = i%module->sideLength; //increment x up to COL length then loop
				if((i%module->sideLength == 0)&&(i!=0)){ //increment y once x hits positive multiple of COL length
					y++;
				}
				
				//nvgFillColor(vg, (module->cells[i] ? nvgRGBA(0,255,0,255) : nvgRGBA(255,0,0,255)));
				if(module->cells[i]){
					nvgFillColor(vg, ((random::uniform() < 0.5) ? nvgRGBA(0,255,0,PIXEL_BRIGHTNESS) : nvgRGBA(0,255,0,PIXEL_BRIGHTNESS+5)));
					nvgBeginPath(vg);
					nvgRect(vg, x*pixelSize, y*pixelSize, pixelSize, pixelSize);
					nvgFill(vg);
				}
				if(i == shadowAntCell){
					if(!module->params[MusicalAnt::SHADOW_ANT_ON].getValue()){
						nvgFillColor(vg, nvgRGBA(0,0,255,255));
						nvgBeginPath(vg);
						nvgRect(vg, x*pixelSize, y*pixelSize, pixelSize, pixelSize);
						nvgFill(vg);
					}
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
				nvgFillColor(vg, ((random::uniform() < 0.5) ? nvgRGBA(0,0,0,0) : nvgRGBA(255,255,255,8)));
				nvgBeginPath(vg);
				nvgRect(vg, x*fuzzPixelSize, y*fuzzPixelSize, fuzzPixelSize, fuzzPixelSize);
				nvgFill(vg);
			}

			// Draw screen reflection over display
			nvgFillColor(vg, nvgRGBA(255,255,255,7));
			nvgBeginPath(vg);
			nvgRect(vg, x*pixelSize, y*pixelSize, pixelSize, pixelSize);
			nvgCircle(vg, 68, 54, 60);
			nvgFill(vg);

			nvgBeginPath(vg);
			nvgRect(vg, x*pixelSize, y*pixelSize, pixelSize, pixelSize);
			nvgCircle(vg, 77, 48, 40);
			nvgFill(vg);

			nvgBeginPath(vg);
			nvgRect(vg, x*pixelSize, y*pixelSize, pixelSize, pixelSize);
			nvgCircle(vg, 82, 43, 20);
			nvgFill(vg);

			nvgFillColor(vg, nvgRGBA(255,255,255,5));
			nvgBeginPath(vg);
			nvgRect(vg, x*pixelSize, y*pixelSize, pixelSize, pixelSize);
			nvgCircle(vg, 87, 40, 8);
			nvgFill(vg);

			nvgFillColor(vg, nvgRGBA(0,0,0,0));

		}

	}

};

struct ParameterFeedbackDisplay : TransparentWidget
{

  MusicalAnt *module;

  int memFont = -1;
  std::string text_label;
  int pxSize;
  int align;
  NVGcolor color;
  std::shared_ptr<Font> font;

  ParameterFeedbackDisplay( MusicalAnt *module, Vec pos, int px, int al, NVGcolor col ) : pxSize( px ), align( al ), color( col )
  {
  	this->module = module;
    box.pos = pos;
    color = col;
    font = APP->window->loadFont(asset::plugin(pluginInstance, "res/DSEG7ClassicMini-Regular.ttf"));
  }

  void draw( NVGcontext *vg ) {
  	if(module) {
	  	text_label = "Ant: " + std::to_string(module->index) + ", Shadow: " + std::to_string(module->shadowIndex);

	    nvgBeginPath( vg );
	    nvgFontFaceId(vg, font->handle);
	    nvgFontSize( vg, pxSize );
	    nvgFillColor( vg, color );
	    nvgTextAlign( vg, align );
	    nvgText( vg, 0, 0, text_label.c_str(), NULL );
	}
  }
};

struct LoopLengthTextLabel : TransparentWidget
{

  MusicalAnt *module;

  int memFont = -1;
  std::string text_label;
  int pxSize;
  int align;
  NVGcolor color;
  std::shared_ptr<Font> font;

  LoopLengthTextLabel( MusicalAnt *module, Vec pos, int px, int al, NVGcolor col ) : pxSize( px ), align( al ), color( col )
  {
  	this->module = module;
    box.pos = pos;
    color = col;
    font = APP->window->loadFont(asset::plugin(pluginInstance, "res/DSEG7ClassicMini-Regular.ttf"));
  }

  void draw( NVGcontext *vg ) {

  	if(module) {
	  	text_label = std::to_string(module->loopLength);

	    nvgBeginPath( vg );
	    nvgFontFaceId(vg, font->handle);
	    nvgFontSize( vg, pxSize );
	    nvgFillColor( vg, color );
	    nvgTextAlign( vg, align );
	    nvgText( vg, 0, 0, text_label.c_str(), NULL );
	}
  }
};


struct MusicalAntWidget : ModuleWidget {
	MusicalAnt *module;


	MusicalAntWidget(MusicalAnt *module) {
		setModule(module);
		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/MusicalAnt.svg")));

		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		// Testing text

		//addChild( new ParameterFeedbackDisplay(module, Vec(13.5, 345), 10, 1, nvgRGBA(255,0,0,255) ) );
		CenteredLabel* const testLabel = new CenteredLabel;
		testLabel->box.pos = Vec(75, 183);
		testLabel->text = "120 BPM";

		addParam(createParam<RoundBlackKnob>(Vec(143.9, 177), module, MusicalAnt::CLOCK_PARAM));

		// Ant X panel widgets
		//addParam(createParam<RoundSmallBlackKnobSnap>(Vec(52.4, 218.5), module, MusicalAnt::OCTAVE_KNOB_PARAM_X));
		RoundSmallBlackKnobSnap *octaveKnobX = dynamic_cast<RoundSmallBlackKnobSnap*>(createParam<RoundSmallBlackKnobSnap>(Vec(52.4, 218.5), module, MusicalAnt::OCTAVE_KNOB_PARAM_X));
		RoundSmallBlackKnobSnap *noteKnobX = dynamic_cast<RoundSmallBlackKnobSnap*>(createParam<RoundSmallBlackKnobSnap>(Vec(76.9, 218.5), module, MusicalAnt::NOTE_KNOB_PARAM_X));
		RoundSmallBlackKnobSnap *scaleKnobX = dynamic_cast<RoundSmallBlackKnobSnap*>(createParam<RoundSmallBlackKnobSnap>(Vec(101.4, 218.5), module, MusicalAnt::SCALE_KNOB_PARAM_X));
		//addParam(createParam<RoundSmallBlackKnobSnap>(Vec(101.4, 218.5), module, MusicalAnt::SCALE_KNOB_PARAM_X));
		addOutput(createOutput<PJ301MPort>(Vec(148.9, 218.5), module, MusicalAnt::VOCT_OUTPUT_X));
		// Invert X V/Oct output
		addParam(createParam<CKSS>(Vec(127.5, 219.75), module, MusicalAnt::VOCT_INVERT_X));
		
		// Ant Y panel widgets
		addParam(createParam<RoundSmallBlackKnobSnap>(Vec(52.4, 254), module, MusicalAnt::OCTAVE_KNOB_PARAM_Y));
		addParam(createParam<RoundSmallBlackKnobSnap>(Vec(76.9, 254), module, MusicalAnt::NOTE_KNOB_PARAM_Y));
		addParam(createParam<RoundSmallBlackKnobSnap>(Vec(101.4, 254), module, MusicalAnt::SCALE_KNOB_PARAM_Y));
		addOutput(createOutput<PJ301MPort>(Vec(148.9, 254), module, MusicalAnt::VOCT_OUTPUT_Y));
		// Invert Y V/Oct output
		addParam(createParam<CKSS>(Vec(127.5, 255.25), module, MusicalAnt::VOCT_INVERT_Y));

		// Shadow Ant X panel widgets
		addParam(createParam<RoundSmallBlackKnobSnap>(Vec(70.9, 289.5), module, MusicalAnt::OCTAVE_KNOB_PARAM_SHADOW_X));
		addParam(createParam<RoundSmallBlackKnobSnap>(Vec(95.4, 289.5), module, MusicalAnt::NOTE_KNOB_PARAM_SHADOW_X));
		addParam(createParam<RoundSmallBlackKnobSnap>(Vec(119.9, 289.5), module, MusicalAnt::SCALE_KNOB_PARAM_SHADOW_X));
		addOutput(createOutput<PJ301MPort>(Vec(148.9, 289.5), module, MusicalAnt::VOCT_OUTPUT_SHADOW_X));

		// Shadow Ant Y panel widgets
		addParam(createParam<RoundSmallBlackKnobSnap>(Vec(70.9, 325), module, MusicalAnt::OCTAVE_KNOB_PARAM_SHADOW_Y));
		addParam(createParam<RoundSmallBlackKnobSnap>(Vec(95.4, 325), module, MusicalAnt::NOTE_KNOB_PARAM_SHADOW_Y));
		addParam(createParam<RoundSmallBlackKnobSnap>(Vec(119.9, 325), module, MusicalAnt::SCALE_KNOB_PARAM_SHADOW_Y));
		addOutput(createOutput<PJ301MPort>(Vec(148.9, 325), module, MusicalAnt::VOCT_OUTPUT_SHADOW_Y));

		

		addInput(createInput<PJ301MPort>(Vec(115.9, 181), module, MusicalAnt::EXT_CLOCK_INPUT));
		addOutput(createOutput<PJ301MPort>(Vec(23.9, 181), module, MusicalAnt::GATE_OUTPUT));

		addOutput(createOutput<PJ301MPort>(Vec(52.9, 181), module, MusicalAnt::GATE_OUTPUT_X));
		addOutput(createOutput<PJ301MPort>(Vec(82.9, 181), module, MusicalAnt::GATE_OUTPUT_Y));
		

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
		addParam(createParam<RoundSmallBlackKnobSnap>(Vec(23.9, 218.5), module, MusicalAnt::SIDE_LENGTH_PARAM));

		// Skip Knob
		addParam(createParam<RoundSmallBlackKnobSnap>(Vec(23.9, 254), module, MusicalAnt::SKIP_PARAM));

		// AuntyLangButton!
		addParam(createParam<AuntyLangButton>(Vec(83.5, 352), module, MusicalAnt::AUNTYLANGBUTTON_PARAM));
		
		if(module){
			octaveKnobX->connectLabel(testLabel, module);
			loopLengthKnob->connectLabel(testLabel, module);
			noteKnobX->connectLabel(testLabel, module);
			scaleKnobX->connectLabel(testLabel, module);
		}
		addChild(testLabel);

		addParam(octaveKnobX);
		addParam(noteKnobX);
		addParam(scaleKnobX);
		addParam(loopLengthKnob);

		// Create display
		ModuleDisplay *display = new ModuleDisplay();
		display->module = module;
		display->box.pos = Vec(DISPLAY_OFFSET_X, DISPLAY_OFFSET_Y);
		display->box.size = Vec(DISPLAY_SIZE_XY, DISPLAY_SIZE_XY);
		addChild(display);

		/*if(module) {
			cout << "display - X: " << module->antVector[X_POSITION] << ", Y: " << module->antVector[Y_POSITION] << "\n";
		}*/
		//addChild(createLight<SmallLight<RedLight>>(Vec((module->antX+1)*6 + DISPLAY_OFFSET_X, (module->antY+1)*6 + DISPLAY_OFFSET_Y), module, (true)));

	}
};


// Specify the Module and ModuleWidget subclass, human-readable
// author name for categorization per pluginInstance, module slug (should never
// change), human-readable module name, and any number of tags
// (found in `include/tags.hpp`) separated by commas.
// TODO Fix up tag
Model *modelMusicalAnt = createModel<MusicalAnt, MusicalAntWidget>("MusicalAnt");
