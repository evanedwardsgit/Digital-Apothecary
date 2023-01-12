/*
* Rue v1.0.0 - Stable Release
* A voltage controlled amplifier with an adjustable response curve and control voltage offset
* Copyright 2023, Evan JJ Edwards - GPLv3 or later
* Contact at eedwards6@wisc.edu with any comments, concerns, or suggestions
*
* My anthem while making this module - Run for Cover by The Killers
*/

#include "plugin.hpp"
#include "DAcomponentlibrary.hpp"
using simd::float_4;

struct Rue : Module {
	enum ParamId {
		CURVE_PARAM,
		GAIN_PARAM,
		OFFSET_PARAM,
		PARAMS_LEN
	};
	enum InputId {
		SIGNAL_INPUT,
		CV_INPUT,
		INPUTS_LEN
	};
	enum OutputId {
		VCA_OUTPUT,
		OUTPUTS_LEN
	};
	enum LightId {
		SIG_LIGHT,
		LIGHTS_LEN
	};

	Rue() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
		configParam(CURVE_PARAM, 1.f, 4.f, 1.f, "x To the ", "-th Power");
		configParam(GAIN_PARAM, 0.0, M_SQRT2, 1.0, "Gain", " dB", -10, 20);
		configParam(OFFSET_PARAM, -5.f, 5.f, 0.f, "Offset", " Volts");
		configInput(SIGNAL_INPUT, "Signal"); 
		configInput(CV_INPUT, "CV");
		configOutput(VCA_OUTPUT, "Signal");
	}

	void process(const ProcessArgs& args) override;
};

void Rue::process(const ProcessArgs &args){
	paramQuantities[GAIN_PARAM]->displayMultiplier = 20 * params[CURVE_PARAM].getValue();
	
	int activeChannels = std::max(1, inputs[SIGNAL_INPUT].getChannels());
	float gain = std::pow(params[GAIN_PARAM].getValue(), params[CURVE_PARAM].getValue());
	float_4 signals[4]; //Is using single instruction, multiple data necessary? nope. Does it make me feel cooler? yep.

	lights[SIG_LIGHT].setBrightness(gain);

	for (int chan = 0; chan < activeChannels; chan+= 4) {
		signals[chan / 4] = inputs[SIGNAL_INPUT].getPolyVoltageSimd<float_4>(chan);

		float_4 amp = gain;

		if (inputs[CV_INPUT].isConnected()) {
			float_4 controlVolt = simd::clamp((inputs[CV_INPUT].getPolyVoltageSimd<float_4>(chan) + params[OFFSET_PARAM].getValue()) / 10.f, 0.f, 1.f);
			amp *= simd::pow(controlVolt, params[CURVE_PARAM].getValue());
		}

		signals[chan / 4] *= amp;

		if(outputs[VCA_OUTPUT].isConnected())
			outputs[VCA_OUTPUT].setVoltageSimd(simd::clamp(signals[chan / 4], -10.f, 10.f), chan); //TODO: Rework so it's not straight up clipping - tanh maybe?
	}

	outputs[VCA_OUTPUT].setChannels(activeChannels);
}

struct RueWidget : ModuleWidget {
	RueWidget(Rue* module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/Panels/Rue.svg")));

		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
	
		addParam(createLightParamCentered<VCVLightSlider<PinkLight>>(mm2px(Vec(5.014, 60.78438)), module, Rue::GAIN_PARAM, Rue::SIG_LIGHT));

		addParam(createParamCentered<Trimpot>(mm2px(Vec(5.014, 22.77)), module, Rue::CURVE_PARAM));
		addParam(createParamCentered<Trimpot>(mm2px(Vec(5.014, 35.51)), module, Rue::OFFSET_PARAM));
		

		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(5.014, 85.303)), module, Rue::CV_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(5.014, 98.043)), module, Rue::SIGNAL_INPUT));

		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(5.014, 110.783)), module, Rue::VCA_OUTPUT));
	}
};

Model* modelRue = createModel<Rue, RueWidget>("Rue");