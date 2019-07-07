#include <rack0.hpp>
#include "QuantizeUtils.cpp"
#include "Logos.cpp"


using namespace rack;

// Forward-declare the Plugin, defined in Template.cpp
extern Plugin *pluginInstance;

// Forward-declare each Model, defined in each module source file
extern Model *modelMusicalAnt;
extern Model *modelBottleRocket;
// TODO extern Model *modelLadyLuck; // Random walk/brownian motion implementation of musical ant

////////////////////////////////////////////// GRID DISPLAY //////////////////////////////////////////////

// Display that can be scaled im resolution

////////////////////////////////////////////// LABELS //////////////////////////////////////////////

/*struct CenteredLabel : Widget {
	std::string text;
	int fontSize;
	CenteredLabel(int _fontSize = 12) {
		fontSize = _fontSize;
		box.size.y = BND_WIDGET_HEIGHT;
	}
	void draw(NVGcontext *vg) override {
		nvgTextAlign(vg, NVG_ALIGN_CENTER);
		nvgFillColor(vg, nvgRGB(25, 150, 252));
		nvgFontSize(vg, fontSize);
		nvgText(vg, box.pos.x, box.pos.y, text.c_str(), NULL);
	}
};*/

////////////////////////////////////////////// BUTTONS //////////////////////////////////////////////
struct CKSS_Horizontal : app::SvgSwitch {
	CKSS_Horizontal() {
		addFrame(APP->window->loadSvg(asset::system("res/CKSS_Horizontal_0.svg")));
		addFrame(APP->window->loadSvg(asset::system("res/CKSS_Horizontal_1.svg")));
	}
};
