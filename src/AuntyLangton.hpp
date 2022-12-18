#include <rack.hpp>
#include "QuantizeUtils.cpp"
#include "Logos.cpp"

#define CELLS 20736
#define LEFT -90
#define RIGHT 90


using namespace rack;

// Forward-declare the Plugin, defined in Template.cpp
extern Plugin *pluginInstance;

// Forward-declare each Model, defined in each module source file
extern Model *modelMusicalAnt;
//extern Model *modelBottleRocket;
// TODO extern Model *modelLadyLuck;  // Name idea
// TODO extern Model *modelNoiseJam;  // Name idea
// TODO Random walk
// TODO brownian motion implementation of musical ant?? 
// TODO arp with brownian motion built in and modulation step lenght?
// TODO Modulo util to wrap V/Oct voltages

////////////////////////////////////////////// SYSTEM STATE //////////////////////////////////////////////

struct MusicalSystemState {
  
  // ant position & direction
  int antX;
  int antY;
  int antDirectionDegrees;

  // shadow ant position & direction
  int shadowAntX;
  int shadowAntY;
  int shadowAntDirectionDegrees;

  std::vector<bool> cells;

  MusicalSystemState() {
  		cells.resize(CELLS, false);
  		antX = 0;
  		antY = 0;
  		antDirectionDegrees = 0;

  		shadowAntX = 0;
  		shadowAntY = 0;
  		shadowAntDirectionDegrees = 0;
  }

};

struct MusicalInstruction {
	std::vector<char> onLight;
	std::vector<char> onDark;
	int lightInstructionPeriod;
	int darkInstructionPeriod;
	int lightInstructionNumber;
	int darkInstructionNumber;

	MusicalInstruction() {
		// Default behaviour
		onLight.push_back('L');
		onDark.push_back('R');
		lightInstructionPeriod = onLight.size();
		darkInstructionPeriod = onDark.size();

		lightInstructionNumber = 0;
		darkInstructionNumber = 0;
	}

	MusicalInstruction(std::string onLightInput, std::string onDarkInput) {
		// Custom behaviour
		int onLightInputSize = onLightInput.size();
		int onDarkInputSize = onDarkInput.size();

		char onLightStr[onLightInputSize + 1];
		onLightInput.copy(onLightStr, onLightInputSize + 1);
		onLightStr[onLightInputSize] = '\0';

		char onDarkStr[onDarkInputSize + 1];
		onDarkInput.copy(onDarkStr, onDarkInputSize + 1);
		onDarkStr[onDarkInputSize] = '\0';

		for(int i = 0;i<onLightInputSize;i++){
			onLight.push_back(onLightStr[i]);
		}

		for(int j = 0;j<onDarkInputSize;j++){
			onDark.push_back(onDarkStr[j]);
		}
		lightInstructionPeriod = onLightInputSize;
		darkInstructionPeriod = onDarkInputSize;

		lightInstructionNumber = 0;
		darkInstructionNumber = 0;
	}

	int instruction(char input) {
		//std::cout << "\nInstruction: " << input << "\n";
		if(input == 'L') {
			return LEFT;
		}
		
		// Implied else
		return RIGHT;

	}

	int getOnLightInstruction() {
		int currentInstructionNumber = lightInstructionNumber;
		lightInstructionNumber = ((lightInstructionNumber + 1) % lightInstructionPeriod);
		return instruction((char) onLight.at(currentInstructionNumber));
	}

	int getOnDarkInstruction() {
		int currentInstructionNumber = darkInstructionNumber;
		//std::cout << "\nFirst Instruction: " << onDark.at(darkInstructionNumber) << "\n";
		darkInstructionNumber = ((darkInstructionNumber + 1) % darkInstructionPeriod);
		//std::cout << "\n2nd Dark Instruciton Number: " << darkInstructionNumber << "\n";
		return instruction((char) onDark.at(currentInstructionNumber));
	}
};

////////////////////////////////////////////// LABELS //////////////////////////////////////////////

struct CenteredLabel : Widget {
	std::string text;
	std::string fontPath = "res/DSEG7ClassicMini-Regular.ttf";
	int fontSize;

	CenteredLabel(int _fontSize = 10) {
		//font = APP->window->loadFont(asset::plugin(pluginInstance, "res/DSEG7ClassicMini-Regular.ttf"));
		//fontSize = _fontSize;
		box.size.y = BND_WIDGET_HEIGHT;
	}
	void draw(const DrawArgs &draw) override {
		std::shared_ptr<Font> font = APP->window->loadFont(fontPath);
		fontSize = 10;
		if(font){
			nvgTextAlign(draw.vg, NVG_ALIGN_CENTER);
			nvgFontFaceId(draw.vg, font->handle);
			nvgFillColor(draw.vg, nvgRGB(0, 0, 0));
			nvgFontSize(draw.vg, fontSize);
			nvgText(draw.vg, box.pos.x, box.pos.y, text.c_str(), NULL );
		}
	}
};

////////////////////////////////////////////// KNOBS //////////////////////////////////////////////

struct RoundSmallBlackKnobSnap : RoundSmallBlackKnob {
	CenteredLabel* linkedLabel = NULL;
	Module* linkedModule = NULL;
	ParamQuantity* paramQuantity = getParamQuantity();

    RoundSmallBlackKnobSnap() {
    	paramQuantity = NULL;
    	snap = true;
    }

    void connectLabel(CenteredLabel* label, Module* module) {
		if(label && module) {
			linkedLabel = label;
			linkedModule = module;
			if (linkedModule && linkedLabel) {
				linkedLabel->text = formatCurrentValue();
			}
		}
	}

	void onChange(const event::Change &e) override {
		RoundSmallBlackKnob::onChange(e);
		if (linkedModule && linkedLabel) {
			linkedLabel->text = formatCurrentValue();
		}
	}

    std::string formatCurrentValue() {
		if(paramQuantity != NULL) {
			return std::to_string(static_cast<unsigned int>(paramQuantity->getDisplayValue()) + 1);
		}
		return "";
	}

};

////////////////////////////////////////////// BUTTONS //////////////////////////////////////////////
struct CKSS_Horizontal : app::SVGSwitch {
	CKSS_Horizontal() {
		addFrame(APP->window->loadSvg(asset::plugin(pluginInstance, "res/CKSS_Horizontal_0.svg")));
		addFrame(APP->window->loadSvg(asset::plugin(pluginInstance, "res/CKSS_Horizontal_1.svg")));
	}
};

struct AuntyLangButton : app::SVGSwitch {
	AuntyLangButton() {
		momentary = true;
		addFrame(APP->window->loadSvg(asset::plugin(pluginInstance, "res/AuntyLangButton_0.svg")));
		addFrame(APP->window->loadSvg(asset::plugin(pluginInstance, "res/AuntyLangButton_1.svg")));
	}
};
