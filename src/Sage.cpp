/*
* Sage v1.1.0 - Stable Release
* A polyphonic, additive harmonic oscillator
* Copyright 2022, Evan JJ Edwards - GPLv3 or later
* Contact at eedwards6@wisc.edu with any comments, concerns, or suggestions
* 
* Pitch calculations and polyphonic structure utilized from Andrew Belt's code (GPLv3) - since 2022
* 
* My anthem while making this module - Be Fine by Madeon
*/

#include "plugin.hpp"
#include "DAcomponentlibrary.hpp"

using simd::float_4;

template <typename T>
struct HarmonicOsc{
	int channels = 0;
	float oddsAmp = 0.f;
	float evensAmp = 0.f;
	T freq = 0.f;
	T phase = 0.f;
	T out = 0.f;

	void process(float sampleTime) {
		phase += freq * sampleTime;
		phase -= simd::floor(phase);
		out = summation(phase);
	}

	T summation(T phase){
		T toReturn = simd::sin(2.f * M_PI * phase);
		
		for (int i = 2; i <= 8; i++){
			if (i % 2 == 0){
				toReturn += evensAmp * simd::sin(2.f * M_PI * phase * i) / i;
			}
			else{
				toReturn += oddsAmp * simd::sin(2.f * M_PI * phase * i) / i;
			}
		}
		return toReturn;
	}
};

struct Sage : Module{
	enum ParamId{
		ODDS_PARAM,
		EVENS_PARAM,
		TUNE_PARAM,
		OUT_PARAM,
		FM_PARAM,
		PARAMS_LEN
	};
	enum InputId{
		FM_INPUT,
		ODDS_INPUT,
		EVENS_INPUT,
		VOCT_INPUT,
		INPUTS_LEN
	};
	enum OutputId{
		OUT_OUTPUT,
		OUTPUTS_LEN
	};
	enum LightId{
		LIGHTS_LEN
	};

	HarmonicOsc<float_4> Oscillators[4];
	int FMmode = 0;

	Sage(){
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
		configParam(ODDS_PARAM, 0.f, 1.f, 0.f, "Odd Harmonics", " %", 0.f, 100.f);
		configParam(EVENS_PARAM, 0.f, 1.f, 0.f, "Even Harmonics", " %", 0.f, 100.f);
		configParam(TUNE_PARAM, -12.f, 12.f, 0.f, "Frequency", " Hz", dsp::FREQ_SEMITONE, dsp::FREQ_C4);
		configParam(OUT_PARAM, 0.f, 1.f, 0.5, "Output Gain", " %", 0.f, 100.f);
		configParam(FM_PARAM, -1.f, 1.f, 0.f, "FM Attenuate", " %", 0.f, 100.f);
		getParamQuantity(FM_PARAM)->randomizeEnabled = false;
		configInput(FM_INPUT, "FM");
		configInput(ODDS_INPUT, "Odds");
		configInput(EVENS_INPUT, "Evens");
		configInput(VOCT_INPUT, "1V/OCT");
		configOutput(OUT_OUTPUT, "Oscillator");
		ResetEvent e;
		onReset(e);
	}

	void onReset(const ResetEvent& e) override {
		Module::onReset(e);
		FMmode = 0;
	}
	
	json_t* dataToJson() override {
		json_t* rootJ = json_object();
		json_object_set_new(rootJ, "FMmode", json_integer(FMmode));
		return rootJ;
	}

	void dataFromJson(json_t* rootJ) override {
		json_t* FMmodeJ = json_object_get(rootJ, "FMmode");
		if (FMmodeJ)
			FMmode = json_integer_value(FMmodeJ);
	}

	void process(const ProcessArgs &args) override;
};


void Sage::process(const ProcessArgs &args){
	float pitchParam = params[TUNE_PARAM].getValue() / 12.f;
	float oddsAmp = params[ODDS_PARAM].getValue();
	float evensAmp = params[EVENS_PARAM].getValue();
	float fmParam = params[FM_PARAM].getValue();
	int activeChannels = std::max(1, inputs[VOCT_INPUT].getChannels());

	if(inputs[ODDS_INPUT].isConnected()){
		oddsAmp = clamp(inputs[ODDS_INPUT].getVoltage() / 15.f + oddsAmp, 0.f, 1.f);
	}
	
	if(inputs[EVENS_INPUT].isConnected()){
		evensAmp = clamp(inputs[EVENS_INPUT].getVoltage() / 15.f + oddsAmp, 0.f, 1.f);
	}

	for (int chan = 0; chan < activeChannels; chan += 4) {
		auto& oscillator = Oscillators[chan / 4];
		oscillator.channels = std::min(activeChannels - chan, 4);
		float_4 pitch = pitchParam + inputs[VOCT_INPUT].getPolyVoltageSimd<float_4>(chan);
		float_4 freq;
		if (FMmode == 0) { //0 is Exp, 1 is linear through-zero
			pitch += inputs[FM_INPUT].getPolyVoltageSimd<float_4>(chan) * fmParam;
			freq = dsp::FREQ_C4 * dsp::approxExp2_taylor5(pitch + 30.f) / std::pow(2.f, 30.f);
			freq = clamp(freq, 0.f, args.sampleRate / 16.f); // nyquist frequency dived by freq of highest harmonic - @48 khz, max should be ~F#7
		}else{
			freq = dsp::FREQ_C4 * dsp::approxExp2_taylor5(pitch + 30.f) / std::pow(2.f, 30.f);
			freq += dsp::FREQ_C4 * inputs[FM_INPUT].getPolyVoltageSimd<float_4>(chan) * fmParam;
			freq = clamp(freq,  -args.sampleRate / 16.f , args.sampleRate / 16.f);
		}

		
		oscillator.freq = freq;
		oscillator.oddsAmp = oddsAmp;
		oscillator.evensAmp = evensAmp;
		oscillator.process(args.sampleTime);

		if (outputs[OUT_OUTPUT].isConnected()) // Rework later for some distortion/saturation instead of straight up clipping
			outputs[OUT_OUTPUT].setVoltageSimd(clamp(oscillator.out * 5.f * params[OUT_PARAM].getValue(), -5.f, 5.f), chan);
	}

	outputs[OUT_OUTPUT].setChannels(activeChannels);
}

struct SageWidget : ModuleWidget{
	SageWidget(Sage *module){
		setModule(module);
		setPanel(createPanel(asset::plugin(pluginInstance, "res/Panels/Sage.svg")));

		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		addParam(createParamCentered<Rogan3PWhite>(mm2px(Vec(8.495, 25.28)), module, Sage::ODDS_PARAM));
		addParam(createParamCentered<Rogan3PWhite>(mm2px(Vec(21.985, 38.77)), module, Sage::EVENS_PARAM));
		
		addParam(createParamCentered<RStyle1PPink>(mm2px(Vec(8.495, 54.515)), module, Sage::TUNE_PARAM));
		addParam(createParamCentered<RStyle1PPink>(mm2px(Vec(21.985, 67.75)), module, Sage::OUT_PARAM));

		addParam(createParamCentered<Trimpot>(mm2px(Vec(8.495, 85.303)), module, Sage::FM_PARAM));

		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(21.985, 85.303)), module, Sage::FM_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(8.495, 98.043)), module, Sage::ODDS_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(21.985, 98.043)), module, Sage::EVENS_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(8.495, 110.783)), module, Sage::VOCT_INPUT));

		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(21.985, 110.783)), module, Sage::OUT_OUTPUT));


	}

	void appendContextMenu(Menu* menu) override {
		Sage* module = dynamic_cast<Sage*>(this->module);

		menu->addChild(new MenuEntry);
		menu->addChild(createMenuLabel("FM"));
		struct ModeItem : MenuItem {
			Sage* module;
			int FMmode;
			void onAction(const event::Action& e) override {
				module->FMmode = FMmode;
			}
		};

		std::string modeNames[2] = {"Exponential", "Linear Through-Zero"};

		for (int i = 0; i < 2; i++) {
			ModeItem* modeItem = createMenuItem<ModeItem>(modeNames[i]);
			modeItem->rightText = CHECKMARK(module->FMmode == i);
			modeItem->module = module;
			modeItem->FMmode = i;
			menu->addChild(modeItem);
		}
	}
};

Model *modelSage = createModel<Sage, SageWidget>("Sage");
