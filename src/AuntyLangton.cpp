#include "AuntyLangton.hpp"


Plugin *pluginInstance;


void init(Plugin *p) {
	pluginInstance = p;

	// Add all Models defined throughout the pluginInstance
	p->addModel(modelMusicalAnt);
	//p->addModel(modelBottleRocket);
	// p->addModel(modelLadyLuck);
	// TODO Shepherd-tonian "wrapping" arpeggiator with CV input to modulate steps amount and other params

	// Any other pluginInstance initialization may go here.
	// As an alternative, consider lazy-loading assets and lookup tables when your module is created to reduce startup times of Rack.
}
