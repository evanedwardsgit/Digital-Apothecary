/*
* Linden v1.0.0 - Stable Release
* A collection of three logic gates, if you can even call them that (you can probably call one that)
* Copyright 2022, Evan JJ Edwards - GPLv3 or later
* Contact at eedwards6@wisc.edu with any comments, concerns, or suggestions
*
* Bernoulli gate, analog logic, and aesthetic of this module inspired by the Kinka, Links, and Branches modules by Mutable Instruments - since 2022
* Implementation of the bernoulli gates is utilized from the structure of Audible Instument's code (GPLv3) - since 2022
*
* My anthem while making this module - 1901 - Phoenix
*/

#include "plugin.hpp"


struct Linden : Module {
	enum ParamId {
		PARAMS_LEN
	};
	enum InputId {
		ANALOG_A_INPUT,
		ANALOG_B_INPUT,
		BERNOULLI_INPUT,
		PROB_INPUT,
		A_CNOT_INPUT,
		B_CNOT_INPUT,
		INPUTS_LEN
	};
	enum OutputId {
		MAX_OUTPUT,
		MIN_OUTPUT,
		A_BERN_OUTPUT,
		B_BERN_OUTPUT,
		A_CNOT_OUTPUT,
		B_CNOT_OUTPUT,
		OUTPUTS_LEN
	};
	enum LightId {
		LIGHTS_LEN
	};

	dsp::BooleanTrigger gateTrig;
	
	Linden() {
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
		configInput(ANALOG_A_INPUT, "Analog A");
		configInput(ANALOG_B_INPUT, "Analog B");
		configOutput(MAX_OUTPUT, "Analog Max");
		configOutput(MIN_OUTPUT, "Analog Min");
		configInput(BERNOULLI_INPUT, "Bernoulli Gate");
		configInput(PROB_INPUT, "Bernoulli Probability");
		configOutput(A_BERN_OUTPUT, "Bernoulli A");
		configOutput(B_BERN_OUTPUT, "Bernoulli B");
		configInput(A_CNOT_INPUT, "CNOT A");
		configInput(B_CNOT_INPUT, "CNOT B");
		configOutput(A_CNOT_OUTPUT, "CNOT A");
		configOutput(B_CNOT_OUTPUT, "CNOT B");
	}

	void process(const ProcessArgs &args) override;
};

void Linden::process(const ProcessArgs &args){
	//Analog "logic"
	outputs[MAX_OUTPUT].setVoltage(inputs[ANALOG_A_INPUT].getVoltage() >  inputs[ANALOG_B_INPUT].getVoltage() ? inputs[ANALOG_A_INPUT].getVoltage() :  inputs[ANALOG_B_INPUT].getVoltage());
	outputs[MIN_OUTPUT].setVoltage(inputs[ANALOG_A_INPUT].getVoltage() <  inputs[ANALOG_B_INPUT].getVoltage() ? inputs[ANALOG_A_INPUT].getVoltage() :  inputs[ANALOG_B_INPUT].getVoltage());	
	
	//Bernoulli gate
	float thresh = 0.5;
	if(inputs[PROB_INPUT].isConnected()){
		thresh = inputs[PROB_INPUT].getVoltage() / 10.f;
	}

	if (gateTrig.process(inputs[BERNOULLI_INPUT].getVoltage() >= 2.f)) {
		bool outcome = random::uniform() < thresh;

		outputs[A_BERN_OUTPUT].setVoltage(!outcome ? 10.f : 0.f);
		outputs[B_BERN_OUTPUT].setVoltage(outcome ? 10.f : 0.f);
	}

	//CNOT

	bool bitA = inputs[A_CNOT_INPUT].getVoltage() >= 2.f;
	bool bitB = inputs[B_CNOT_INPUT].getVoltage() >= 2.f;
	bool outA, outB;

	if(!bitA && !bitB){
		outA = false;
		outB = false;
	}else if(!bitA && bitB){
		outA = false;
		outB = true;
	}else if(bitA && !bitB){
		outA = true;
		outB = true;
	}else{
		outA = true;
		outB = false;
	}

	outputs[A_CNOT_OUTPUT].setVoltage(outA ? 10.f : 0.f);
	outputs[B_CNOT_OUTPUT].setVoltage(outB ? 10.f : 0.f);
	
}

struct LindenWidget : ModuleWidget {
	LindenWidget(Linden* module) {
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/Panels/Linden.svg")));

		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(5.955, 26.18402)), module, Linden::ANALOG_A_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(14.365, 26.18402)), module, Linden::ANALOG_B_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(5.955, 62.11351)), module, Linden::BERNOULLI_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(14.365, 62.11351)), module, Linden::PROB_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(5.955, 98.04299)), module, Linden::A_CNOT_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(14.365, 98.04299)), module, Linden::B_CNOT_INPUT));

		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(5.955, 38.92401)), module, Linden::MAX_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(14.365, 38.92401)), module, Linden::MIN_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(5.955, 74.96958)), module, Linden::A_BERN_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(14.365, 74.96958)), module, Linden::B_BERN_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(5.955, 110.78298)), module, Linden::A_CNOT_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(14.365, 110.78298)), module, Linden::B_CNOT_OUTPUT));
	}
};

Model* modelLinden = createModel<Linden, LindenWidget>("Linden");
