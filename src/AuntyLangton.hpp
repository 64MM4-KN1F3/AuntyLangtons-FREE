#include <rack.hpp>
#include "QuantizeUtils.cpp"
#include "Logos.cpp"


using namespace rack;

// Forward-declare the Plugin, defined in Template.cpp
extern Plugin *pluginInstance;

// Forward-declare each Model, defined in each module source file
extern Model *modelMusicalAnt;
extern Model *modelBottleRocket;
// TODO extern Model *modelLadyLuck;  // Name idea
// TODO extern Model *modelNoiseJam;  // Name idea
// TODO Random walk
// TODO brownian motion implementation of musical ant?? 
// TODO arp with brownian motion built in and modulation step lenght?
// TODO Modulo util to wrap V/Oct voltages

////////////////////////////////////////////// GRID DISPLAY //////////////////////////////////////////////

// Display that can be scaled im resolution

////////////////////////////////////////////// LABELS //////////////////////////////////////////////

struct CenteredLabel : Widget {
	std::string text;
	int fontSize;
	CenteredLabel(int _fontSize = 12) {
		fontSize = _fontSize;
		box.size.y = BND_WIDGET_HEIGHT;
	}
	void draw(const DrawArgs &args) override {
		nvgTextAlign(args.vg, NVG_ALIGN_CENTER);
		nvgFillColor(args.vg, nvgRGB(25, 150, 252));
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
		if(paramQuantity != NULL){
			return std::to_string(static_cast<unsigned int>(paramQuantity->getValue()));
		}
		return "";
	}

};

////////////////////////////////////////////// BUTTONS //////////////////////////////////////////////
struct CKSS_Horizontal : SVGSwitch {
	CKSS_Horizontal() {
		addFrame(APP->window->loadSvg(asset::plugin(pluginInstance, "res/CKSS_Horizontal_0.svg")));
		addFrame(APP->window->loadSvg(asset::plugin(pluginInstance, "res/CKSS_Horizontal_1.svg")));
	}
};

struct AuntyLangButton : SVGSwitch {
	AuntyLangButton() {
		momentary = true;
		addFrame(APP->window->loadSvg(asset::plugin(pluginInstance, "res/AuntyLangButton_0.svg")));
		addFrame(APP->window->loadSvg(asset::plugin(pluginInstance, "res/AuntyLangButton_1.svg")));
	}
};
