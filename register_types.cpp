/* register_types.cpp */

#include "register_types.h"

#include "core/object/class_db.h"
#include "audio_effect_convolution_reverb.h"

void initialize_audio_effect_convolution_reverb_module(ModuleInitializationLevel p_level) {
	if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
		return;
	}
	ClassDB::register_class<AudioEffectConvolutionReverb>();
}

void uninitialize_audio_effect_convolution_reverb_module(ModuleInitializationLevel p_level) {
	if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
		return;
	}
   // Nothing to do here in this example.
}
