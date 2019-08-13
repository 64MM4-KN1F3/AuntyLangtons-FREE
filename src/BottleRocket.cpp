#include "AuntyLangton.hpp"
#include <iostream>
#include <vector>
using namespace std;

#define TOTAL_CARGO_HOLDS 6


struct BottleRocket : Module {
	enum ParamIds {
		PITCH_INPUT_PARAM,
		NUM_PARAMS
	};
	enum InputIds {
		PITCH_INPUT,
		NUM_INPUTS
	};
	enum OutputIds {
		ENUMS(PITCH_OUTPUT, TOTAL_CARGO_HOLDS),
		NUM_OUTPUTS
	};
	enum LightIds {
		BLINK_LIGHT,
		NUM_LIGHTS
	};

	float phase = 0.0;
	float blinkPhase = 0.0;
	float lastInputPitch = -1.0;
	//float pitchStore[TOTAL_CARGO_HOLDS] = {-1.0, -1.0, -1.0, -1.0, -1.0, -1.0};
	//vector<int> *v = new vector<int>(10); (TOTAL_CARGO_HOLDS, -1.0);
	vector<float> *pitchStore;

	BottleRocket() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
		configParam(BottleRocket::PITCH_INPUT_PARAM, -3.0, 3.0, 0.0, "");
		pitchStore = new vector<float>(TOTAL_CARGO_HOLDS, -1.0);
	}
	void process(const ProcessArgs &args) override;
	bool fillPitchStore(float in) {
		int i = 0;
		for(i = 0; i < TOTAL_CARGO_HOLDS; i++) {
			if ((pitchStore->at(i) != in) && (pitchStore->at(i) < 0)) {
				pitchStore->at(i) = in;
				cout << "Just added " << in << " to pitchStore[" << i << "]\n";
				break;
			}
		}
		if(i == 5) {
			return true;
		}
		else {
			return false;
		}
	}
	void clearPitchStore() {
		cout << "Clearing cargo hold\n";
		pitchStore->assign(TOTAL_CARGO_HOLDS, -1.0);
	}
	float getLastPitch() {
		return lastInputPitch;
	}
	void setLastPitch(float in) {
		lastInputPitch = in;
	}

	// For more advanced Module features, read Rack's engine.hpp header file
	// - dataToJson, dataFromJson: serialization of internal data
	// - onSampleRateChange: event triggered by a change of sample rate
	// - onReset, onRandomize, onCreate, onDelete: implements special behavior when user clicks these from the context menu
};


void BottleRocket::process(const ProcessArgs &args) {
	// Implement a simple sine oscillator
	float deltaTime = args.sampleTime;

	// Compute the frequency from the pitch parameter and input
	float pitch = params[PITCH_INPUT_PARAM].getValue();
	pitch += inputs[PITCH_INPUT].getVoltage();
	pitch = clamp(pitch, -4.0f, 4.0f);
	// The default pitch is C4
	float freq = 261.626f * powf(2.0f, pitch);

	// Accumulate the phase
	phase += freq * deltaTime;
	if (phase >= 1.0f)
		phase -= 1.0f;

	// Compute the sine output
	float sine = sinf(2.0f * M_PI * phase);
	//outputs[SINE_OUTPUT].setVoltage(5.0f * sine);

	// Blink light at 1Hz
	blinkPhase += deltaTime;
	if (blinkPhase >= 1.0f)
		blinkPhase -= 1.0f;
	lights[BLINK_LIGHT].value = (blinkPhase < 0.5f) ? 1.0f : 0.0f;

	// Bottle Rocket Step Code
	float epsilon = 0.000001;
	if(abs(inputs[PITCH_INPUT].getVoltage() - getLastPitch()) > epsilon){
		setLastPitch(inputs[PITCH_INPUT].getVoltage());
		cout << "The input Pitch has changed.\n";
		cout << "H@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@RRRGH!!!!";
			if(fillPitchStore(inputs[PITCH_INPUT].getVoltage())){
			for(int i = 0; i < TOTAL_CARGO_HOLDS; i++) {
				outputs[PITCH_OUTPUT + i].setVoltage(pitchStore->at(i));
			}
			clearPitchStore();
			//cout << "H@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@RRRGH!!!!";
		}
	}
	else {/*
		cout << "The input Pitch has not be detected as changed.\n";
		cout << "inputs[PITCH_INPUT].getVoltage(): " << inputs[PITCH_INPUT].getVoltage() << "\n";
		cout << "getLastPitch(): " << getLastPitch() << "\n";
		cout << "abs(inputs[PITCH_INPUT].getVoltage() - getLastPitch()): " << abs(inputs[PITCH_INPUT].getVoltage() - getLastPitch()) << "\n";
	*/}


	

}


struct BottleRocketWidget : ModuleWidget {
	BottleRocketWidget(BottleRocket *module) {
		setModule(module);
		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/BottleRocket.svg")));

		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		addParam(createParam<Davies1900hBlackKnob>(Vec(28, 87), module, BottleRocket::PITCH_INPUT_PARAM));

		addInput(createInput<PJ301MPort>(Vec(16, 40), module, BottleRocket::PITCH_INPUT));

		// 6 Pitch Outputs
		addOutput(createOutput<PJ301MPort>(Vec(33, 70), module, BottleRocket::PITCH_OUTPUT));
		addOutput(createOutput<PJ301MPort>(Vec(33, 110), module, BottleRocket::PITCH_OUTPUT + 1));
		addOutput(createOutput<PJ301MPort>(Vec(33, 150), module, BottleRocket::PITCH_OUTPUT + 2));
		addOutput(createOutput<PJ301MPort>(Vec(33, 190), module, BottleRocket::PITCH_OUTPUT + 3));
		addOutput(createOutput<PJ301MPort>(Vec(33, 230), module, BottleRocket::PITCH_OUTPUT + 4));
		addOutput(createOutput<PJ301MPort>(Vec(33, 270), module, BottleRocket::PITCH_OUTPUT + 5));


		addChild(createLight<MediumLight<RedLight>>(Vec(41, 59), module, BottleRocket::BLINK_LIGHT));
	}
};


// Specify the Module and ModuleWidget subclass, human-readable
// author name for categorization per pluginInstance, module slug (should never
// change), human-readable module name, and any number of tags
// (found in `include/tags.hpp`) separated by commas.
// TODO Fix up tag
Model *modelBottleRocket = createModel<BottleRocket, BottleRocketWidget>("BottleRocket");
