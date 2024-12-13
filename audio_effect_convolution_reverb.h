/* audio_effect_convolution_reverb.h */

#ifndef AUDIO_EFFECT_CONVOLUTION_REVERB_H
#define AUDIO_EFFECT_CONVOLUTION_REVERB_H

#include "core/object/class_db.h"
#include "core/object/ref_counted.h"
#include "core/templates/vector.h"
#include "scene/resources/audio_stream_wav.h" // For AudioStreamWAV
#include "servers/audio/audio_effect.h"

#include <fftw3.h>

class AudioEffectConvolutionReverb;

class AudioEffectConvolutionReverbInstance : public AudioEffectInstance {
	GDCLASS(AudioEffectConvolutionReverbInstance, AudioEffectInstance);

private:
	Ref<AudioEffectConvolutionReverb> base;

	// FFTW related
	int fft_size = 0;
	Vector<double> input_buffer;
	Vector<double> output_buffer;
	Vector<double> overlap_buffer;

	fftw_complex *input_freq = nullptr;
	fftw_complex *output_freq = nullptr;

	fftw_plan forward_plan = nullptr;
	fftw_plan inverse_plan = nullptr;

	void setup_fftw(int p_frame_count);
	void cleanup_fftw();

public:
	void set_base(const Ref<AudioEffectConvolutionReverb> &p_base);

	virtual void process(const AudioFrame *p_src_frames, AudioFrame *p_dst_frames, int p_frame_count) override;

	AudioEffectConvolutionReverbInstance();
	~AudioEffectConvolutionReverbInstance();
};

class AudioEffectConvolutionReverb : public AudioEffect {
	GDCLASS(AudioEffectConvolutionReverb, AudioEffect);

private:
	Ref<AudioStreamWAV> impulse_response;
	fftw_complex *ir_freq = nullptr;
	int ir_length = 0; // Length of the impulse response in samples

	float gain = 0.5f; // Default gain set to 0.5
	float dry = 0.0f; // Dry signal level
	float wet = 1.0f; // Wet signal level
	bool auto_gain = true;

public:
	void set_impulse_response(const Ref<AudioStreamWAV> &p_impulse_response);
	Ref<AudioStreamWAV> get_impulse_response() const;

	fftw_complex *get_ir_freq() const;
	int get_ir_length() const;

	void set_gain(float p_gain);
	float get_gain() const;

	void set_dry(float p_dry);
	float get_dry() const;

	void set_wet(float p_wet);
	float get_wet() const;

	void set_auto_gain(bool p_auto_gain);
	bool is_auto_gain() const;

	virtual Ref<AudioEffectInstance> instantiate() override;

	AudioEffectConvolutionReverb();
	~AudioEffectConvolutionReverb();

protected:
	static void _bind_methods();
};

#endif // AUDIO_EFFECT_CONVOLUTION_REVERB_H
