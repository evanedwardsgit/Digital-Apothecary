/*
* Sage v1.0.0 - Stable Release
* A polyphonic, additive harmonic oscillator
* Copyright 2022, Evan JJ Edwards - GPLv3 or later
* Contact at eedwards6@wisc.edu with any comments, concerns, or suggestions
* 
* Pitch calculations and polyphonic structure utilized from Andrew Belt's code (GPLv3) - 2022
*
* My anthem while making this module - Be Fine by Madeon
*/

#include "plugin.hpp"
#include "DAcomponentlibrary.hpp"

using simd::float_4;

template <typename T>
struct HarmonicOsc{
	int channels;
	float oddsAmp, evensAmp;
	T freq, phase;
	T out;

	public:
		HarmonicOsc(){
			reset();
		}

		void reset(){
			phase = 0.f;
			freq = 0.f;
			out = 0.f;
			channels = 0;
			oddsAmp = 0.f;
			evensAmp = 0.f;
		}

		void process(float sampleTime) {
			phase += freq * sampleTime;
			phase -= simd::floor(phase);
			out = summation(phase);
		}

	private:
		T summation(T phase){
			// No need to iterate if amplitudes are zero, saves resources - especially with many polyphonic channels 
			if(oddsAmp == 0.f && evensAmp == 0.f){
				return simd::sin(2.f * M_PI * phase);
			}

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
	Sage(){
		config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
		configParam(ODDS_PARAM, 0.f, 1.f, 0.f, "Odd Harmonics", " %", 0.f, 100.f);
		configParam(EVENS_PARAM, 0.f, 1.f, 0.f, "Even Harmonics", " %", 0.f, 100.f);
		configParam(TUNE_PARAM, -12.f, 12.f, 0.f, "Frequency", " Hz", dsp::FREQ_SEMITONE, dsp::FREQ_C4);
		configParam(OUT_PARAM, 0.f, 1.f, 0.5, "Output Gain", " %", 0.f, 100.f);
		configParam(FM_PARAM, -1.f, 1.f, 0.f, "FM Attenuate", " %", 0.f, 100.f);
		getParamQuantity(FM_PARAM)->randomizeEnabled = false;
		configInput(FM_INPUT, "FM Modulation");
		configInput(ODDS_INPUT, "ODDS");
		configInput(EVENS_INPUT, "EVENS");
		configInput(VOCT_INPUT, "1V/OCT");
		configOutput(OUT_OUTPUT, "OUTPUT");
	}

	void process(const ProcessArgs &args) override;
};


void Sage::process(const ProcessArgs &args){
	float pitchParam = params[TUNE_PARAM].getValue() / 12.f;
	float oddsAmp = params[ODDS_PARAM].getValue();
	float evensAmp = params[EVENS_PARAM].getValue();
	float fmParam = params[FM_PARAM].getValue();
	int activeChannels = std::max(1, inputs[VOCT_INPUT].getChannels());\

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
		pitch += inputs[FM_INPUT].getPolyVoltageSimd<float_4>(chan) * fmParam;
		pitch = dsp::FREQ_C4 * dsp::approxExp2_taylor5(pitch + 30.f) / std::pow(2.f, 30.f);
		float_4 freq = clamp(pitch, 0.f, args.sampleRate / 16.f); // nyquist frequency dived by freq of highest harmonic - @48 khz, max should be ~F#7
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
};

Model *modelSage = createModel<Sage, SageWidget>("Sage");