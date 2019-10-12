#include <rack.hpp>
#include "QuantizeUtils.cpp"

//#include "Logos.cpp"


#define CELLS 20736
#define L -90
#define R 90


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

	int Instruction(char input) {
		if(input == 'L')
			return L;
		if(input == 'R')
			return R;
	}

	int getOnLightInstruction() {
		char step = onLight.at(lightInstructionNumber);
		lightInstructionNumber = ((lightInstructionNumber + 1) % lightInstructionPeriod) - 1;
		return Instruction(step);
	}

	int getOnDarkInstruction() {
		char step = onDark.at(darkInstructionNumber);
		darkInstructionNumber = ((darkInstructionNumber + 1) % darkInstructionPeriod) - 1;
		return Instruction(step);
	}
};

////////////////////////////////////////////// LABELS //////////////////////////////////////////////

struct CenteredLabel : Widget {
	std::string text;
	int fontSize;
	std::shared_ptr<Font> font;

	CenteredLabel(int _fontSize = 10) {
		font = APP->window->loadFont(asset::plugin(pluginInstance, "res/DSEG7ClassicMini-Regular.ttf"));
		fontSize = _fontSize;
		box.size.y = BND_WIDGET_HEIGHT;
	}
	void draw(const DrawArgs &args) override {
		nvgTextAlign(args.vg, NVG_ALIGN_CENTER);
		nvgFontFaceId(args.vg, font->handle);
		nvgFillColor(args.vg, nvgRGB(0, 0, 0));
		nvgFontSize(args.vg, fontSize);
		nvgText(args.vg, box.pos.x, box.pos.y, text.c_str(), NULL);
	}
};

////////////////////////////////////////////// KNOBS //////////////////////////////////////////////

struct RoundSmallBlackKnobSnap : RoundSmallBlackKnob {
	CenteredLabel* linkedLabel = NULL;
	Module* linkedModule = NULL;

    RoundSmallBlackKnobSnap() {
        paramQuantity = NULL;
        snap = true;
    }

    void connectLabel(CenteredLabel* label, Module* module) {
		linkedLabel = label;
		linkedModule = module;
		if (linkedModule && linkedLabel) {
			linkedLabel->text = formatCurrentValue();
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
