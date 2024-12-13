/* audio_effect_convolution_reverb.cpp */

#include "audio_effect_convolution_reverb.h"

#include "core/error/error_macros.h"
#include "core/object/class_db.h"
#include "servers/audio_server.h"

#include <fftw3.h>
#include <algorithm> // For std::max
#include <cmath> // For sqrt

// Constructor
AudioEffectConvolutionReverb::AudioEffectConvolutionReverb() {
	// Initialization done in member variable declarations
}

// Destructor
AudioEffectConvolutionReverb::~AudioEffectConvolutionReverb() {
	if (ir_freq) {
		fftw_free(ir_freq);
		ir_freq = nullptr;
	}
}

// Set impulse response
void AudioEffectConvolutionReverb::set_impulse_response(const Ref<AudioStreamWAV> &p_impulse_response) {
	impulse_response = p_impulse_response;

	if (!impulse_response.is_valid()) {
		ERR_PRINT("Invalid impulse response provided.");
		ir_length = 0;
		return;
	}

	// Check that the sample format is 16-bit PCM
	// if (impulse_response->get_format() != AudioStreamWAV::FORMAT_PCM_16_BITS) {
	// 	ERR_PRINT("Impulse response must be in 16-bit PCM format.");
	// 	ir_length = 0;
	// 	return;
	// }

	// Get sample data from the AudioStreamWAV
	PackedByteArray ir_samples = impulse_response->get_data();
	ir_length = ir_samples.size() / sizeof(int16_t);

	if (ir_length <= 0) {
		ERR_PRINT("Impulse response data is empty.");
		ir_length = 0;
		return;
	}

	// Determine FFT size for the impulse response (next power of two)
	int ir_fft_size = 1;
	while (ir_fft_size < ir_length) {
		ir_fft_size <<= 1;
	}

	// Prepare buffer for FFT
	Vector<double> ir_buffer;
	ir_buffer.resize(ir_fft_size);

	// Read samples and convert to double in range [-1.0, 1.0]
	const int16_t *ir_samples_data = reinterpret_cast<const int16_t *>(ir_samples.ptr());
	double *ir_buffer_ptr = ir_buffer.ptrw();

	// Compute total energy and populate the buffer
	double energy = 0.0;
	for (int i = 0; i < ir_length; i++) {
		double sample = ir_samples_data[i] / 32768.0;
		energy += sample * sample;
		ir_buffer_ptr[i] = sample;
	}

	// Avoid division by zero
	if (energy == 0.0) {
		ERR_PRINT("Impulse response energy is zero.");
		ir_length = 0;
		return;
	}

	// Normalize the impulse response to unit energy
	double norm_factor = sqrt(energy);
	for (int i = 0; i < ir_length; i++) {
		ir_buffer_ptr[i] /= norm_factor;
	}

	// Zero the rest of the buffer (if any)
	for (int i = ir_length; i < ir_fft_size; i++) {
		ir_buffer_ptr[i] = 0.0;
	}

	// Allocate FFTW array for impulse response
	if (ir_freq) {
		fftw_free(ir_freq);
	}
	ir_freq = (fftw_complex *)fftw_malloc(sizeof(fftw_complex) * (ir_fft_size / 2 + 1));

	// Create FFTW plan for impulse response
	fftw_plan ir_plan = fftw_plan_dft_r2c_1d(ir_fft_size, ir_buffer_ptr, ir_freq, FFTW_ESTIMATE);

	// Execute FFT for the impulse response
	fftw_execute(ir_plan);

	// Clean up plan
	fftw_destroy_plan(ir_plan);
}

// Get impulse response
Ref<AudioStreamWAV> AudioEffectConvolutionReverb::get_impulse_response() const {
	return impulse_response;
}

// Get impulse response frequency data
fftw_complex *AudioEffectConvolutionReverb::get_ir_freq() const {
	return ir_freq;
}

int AudioEffectConvolutionReverb::get_ir_length() const {
	return ir_length;
}

void AudioEffectConvolutionReverb::set_gain(float p_gain) {
	gain = p_gain;
}

float AudioEffectConvolutionReverb::get_gain() const {
	return gain;
}

void AudioEffectConvolutionReverb::set_dry(float p_dry) {
	dry = CLAMP(p_dry, 0.0f, 1.0f);
}

float AudioEffectConvolutionReverb::get_dry() const {
	return dry;
}

void AudioEffectConvolutionReverb::set_wet(float p_wet) {
	wet = CLAMP(p_wet, 0.0f, 1.0f);
}

float AudioEffectConvolutionReverb::get_wet() const {
	return wet;
}

void AudioEffectConvolutionReverb::set_auto_gain(bool p_auto_gain) {
	auto_gain = p_auto_gain;
}

bool AudioEffectConvolutionReverb::is_auto_gain() const {
	return auto_gain;
}

// Instantiate effect instance
Ref<AudioEffectInstance> AudioEffectConvolutionReverb::instantiate() {
	Ref<AudioEffectConvolutionReverbInstance> ins;
	ins.instantiate();
	ins->set_base(this);
	return ins;
}

// Bind methods
void AudioEffectConvolutionReverb::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_impulse_response", "impulse_response"), &AudioEffectConvolutionReverb::set_impulse_response);
	ClassDB::bind_method(D_METHOD("get_impulse_response"), &AudioEffectConvolutionReverb::get_impulse_response);

	ClassDB::bind_method(D_METHOD("set_gain", "gain"), &AudioEffectConvolutionReverb::set_gain);
	ClassDB::bind_method(D_METHOD("get_gain"), &AudioEffectConvolutionReverb::get_gain);

	ClassDB::bind_method(D_METHOD("set_dry", "dry"), &AudioEffectConvolutionReverb::set_dry);
	ClassDB::bind_method(D_METHOD("get_dry"), &AudioEffectConvolutionReverb::get_dry);

	ClassDB::bind_method(D_METHOD("set_wet", "wet"), &AudioEffectConvolutionReverb::set_wet);
	ClassDB::bind_method(D_METHOD("get_wet"), &AudioEffectConvolutionReverb::get_wet);

	ClassDB::bind_method(D_METHOD("set_auto_gain", "auto_gain"), &AudioEffectConvolutionReverb::set_auto_gain);
	ClassDB::bind_method(D_METHOD("is_auto_gain"), &AudioEffectConvolutionReverb::is_auto_gain);

	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "impulse_response", PROPERTY_HINT_RESOURCE_TYPE, "AudioStreamWAV"), "set_impulse_response", "get_impulse_response");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "gain", PROPERTY_HINT_RANGE, "0.0,10.0,0.01"), "set_gain", "get_gain");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "dry", PROPERTY_HINT_RANGE, "0.0,1.0,0.01"), "set_dry", "get_dry");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "wet", PROPERTY_HINT_RANGE, "0.0,1.0,0.01"), "set_wet", "get_wet");
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "auto_gain"), "set_auto_gain", "is_auto_gain");
}

// Constructor
AudioEffectConvolutionReverbInstance::AudioEffectConvolutionReverbInstance() {
	// Initialization done in member variable declarations
}

// Destructor
AudioEffectConvolutionReverbInstance::~AudioEffectConvolutionReverbInstance() {
	cleanup_fftw();
}

// Set base effect
void AudioEffectConvolutionReverbInstance::set_base(const Ref<AudioEffectConvolutionReverb> &p_base) {
	base = p_base;
}

// FFTW setup
void AudioEffectConvolutionReverbInstance::setup_fftw(int p_frame_count) {
	int ir_length = base->get_ir_length();
	int required_size = p_frame_count + ir_length - 1;

	if (required_size <= 0) {
		ERR_PRINT("Invalid required size for FFT setup.");
		return;
	}

	fft_size = 1;
	while (fft_size < required_size) {
		fft_size <<= 1;
	}

	// Allocate buffers
	input_buffer.resize(fft_size);
	output_buffer.resize(fft_size);
	overlap_buffer.resize(fft_size - p_frame_count);

	// Allocate FFTW arrays
	input_freq = (fftw_complex *)fftw_malloc(sizeof(fftw_complex) * (fft_size / 2 + 1));
	output_freq = (fftw_complex *)fftw_malloc(sizeof(fftw_complex) * (fft_size / 2 + 1));

	// Create FFTW plans
	forward_plan = fftw_plan_dft_r2c_1d(fft_size, input_buffer.ptrw(), input_freq, FFTW_ESTIMATE);
	inverse_plan = fftw_plan_dft_c2r_1d(fft_size, output_freq, output_buffer.ptrw(), FFTW_ESTIMATE);
}

// FFTW cleanup
void AudioEffectConvolutionReverbInstance::cleanup_fftw() {
	// Destroy FFTW plans
	if (forward_plan) {
		fftw_destroy_plan(forward_plan);
		forward_plan = nullptr;
	}
	if (inverse_plan) {
		fftw_destroy_plan(inverse_plan);
		inverse_plan = nullptr;
	}

	// Free FFTW arrays
	if (input_freq) {
		fftw_free(input_freq);
		input_freq = nullptr;
	}
	if (output_freq) {
		fftw_free(output_freq);
		output_freq = nullptr;
	}

	// Clear buffers
	input_buffer.clear();
	output_buffer.clear();
	overlap_buffer.clear();
}

// Process audio frames
void AudioEffectConvolutionReverbInstance::process(const AudioFrame *p_src_frames, AudioFrame *p_dst_frames, int p_frame_count) {
	if (!base.is_valid() || base->get_ir_freq() == nullptr || base->get_ir_length() == 0) {
		// Impulse response not set or invalid, pass input to output
		for (int i = 0; i < p_frame_count; i++) {
			p_dst_frames[i] = p_src_frames[i];
		}
		return;
	}

	int ir_length = base->get_ir_length();
	int required_size = p_frame_count + ir_length - 1;

	if (fft_size < required_size) {
		cleanup_fftw();
		setup_fftw(p_frame_count);
	}

	if (fft_size <= 0) {
		ERR_PRINT("FFT size is invalid.");
		for (int i = 0; i < p_frame_count; i++) {
			p_dst_frames[i] = p_src_frames[i];
		}
		return;
	}

	// Obtain pointers
	double *input_buffer_ptr = input_buffer.ptrw();
	double *output_buffer_ptr = output_buffer.ptrw();
	double *overlap_buffer_ptr = overlap_buffer.ptrw();

	// Copy input frames to input buffer (convert to mono)
	for (int i = 0; i < p_frame_count; i++) {
		// Average the left and right channels to get mono input
		input_buffer_ptr[i] = (p_src_frames[i].l + p_src_frames[i].r) * 0.5;
	}

	// Zero the rest of the input buffer
	for (int i = p_frame_count; i < fft_size; i++) {
		input_buffer_ptr[i] = 0.0;
	}

	// Perform forward FFT on input buffer
	fftw_execute(forward_plan);

	int conv_size = fft_size / 2 + 1;

	// Convolve in frequency domain
	fftw_complex *ir_freq = base->get_ir_freq();
	for (int i = 0; i < conv_size; i++) {
		double a = input_freq[i][0];
		double b = input_freq[i][1];
		double c = ir_freq[i][0];
		double d = ir_freq[i][1];

		// Complex multiplication: (a + ib) * (c + id) = (ac - bd) + i(ad + bc)
		output_freq[i][0] = a * c - b * d;
		output_freq[i][1] = a * d + b * c;
	}

	// Perform inverse FFT
	fftw_execute(inverse_plan);

	// Normalize the output
	for (int i = 0; i < fft_size; i++) {
		output_buffer_ptr[i] /= fft_size;
	}

	// Add overlap from previous block
	for (int i = 0; i < overlap_buffer.size(); i++) {
		output_buffer_ptr[i] += overlap_buffer_ptr[i];
	}

	// Save new overlap for next block
	int overlap_length = fft_size - p_frame_count;
	for (int i = 0; i < overlap_length; i++) {
		overlap_buffer_ptr[i] = output_buffer_ptr[p_frame_count + i];
	}

	// Retrieve gain and mix parameters
	float gain = base->get_gain();
	float dry = base->get_dry();
	float wet = base->get_wet();
	bool auto_gain = base->is_auto_gain();

	// Apply automatic gain compensation if enabled
	if (auto_gain && ir_length > 0) {
		gain /= sqrt((double)ir_length);
	}

	// Apply gain, mix dry and wet signals, and copy to output
	for (int i = 0; i < p_frame_count; i++) {
		double wet_sample = output_buffer_ptr[i] * gain;
		double dry_sample = (p_src_frames[i].l + p_src_frames[i].r) * 0.5;

		double sample = dry_sample * dry + wet_sample * wet;

		// Clipping
		if (sample > 1.0)
			sample = 1.0;
		else if (sample < -1.0)
			sample = -1.0;

		p_dst_frames[i].l = sample;
		p_dst_frames[i].r = sample;
	}
}
