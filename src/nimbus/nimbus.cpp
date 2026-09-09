#include <math.h>

#include "../../lib/loewy.h"
#include "../../lib/utils.h"
#include "daisysp.h"
#include "granular_processor.h"

using namespace daisy;
using namespace loewy;

/*
 * Author: Ben van der Burgh
 *
 * Nimbus (Electro-Smith's port of Mutable Instruments Clouds) for the
 * Löwenzahnhonig. Runs Clouds' granular mode in 16-bit stereo quality, always
 * fully wet; mix the dry signal in outside the module if you need it.
 *
 * Pot 1: Position. Where in the recording buffer the grains are taken from:
 *        fully CCW is the most recent audio, clockwise travels back in time
 *        (up to about 0.7 s). Scrubs through the buffer while frozen.
 * Pot 2: Size. Grain length, about 20 ms to 340 ms.
 * Pot 3: Pitch. -2 to +2 octaves, exactly unison around the centre.
 * Pot 4: Density, as on Clouds: regular grains left of the centre, none at
 *        the centre, random grains right of it, more overlap towards the ends.
 *        The right half also smooths the grain envelopes, engages the diffuser
 *        and adds reverb, so fully CW is a thick diffuse wash.
 * CV 1:  Position, added to Pot 1.
 * CV 2:  Freeze/trigger gate. The buffer is frozen while the gate is high and
 *        every rising edge plays one grain, which is what you hear when Pot 4
 *        is centred (a trigger sampler, like Clouds' TRIG input).
 */

namespace {

// Pitch pot: quadratic curve for fine control near unison, with a dead zone so
// that a roughly centred pot is exactly unison.
constexpr float kPitchDeadZone = 0.04f;
constexpr float kPitchRangeSemitones = 24.f;

// Freeze gate thresholds on the 0..1 CV reading, with hysteresis.
constexpr float kFreezeOn = 0.55f;
constexpr float kFreezeOff = 0.45f;

// Blend parameters without a control of their own.
constexpr float kDryWet = 1.f;
constexpr float kStereoSpread = 0.5f;
constexpr float kFeedback = 0.15f;

float PitchFromPot(float pot) {
  float offset = pot - 0.5f;
  float magnitude = fabsf(offset) - kPitchDeadZone;
  if (magnitude <= 0.f) return 0.f;
  float x = magnitude / (0.5f - kPitchDeadZone);
  float semitones = kPitchRangeSemitones * x * x;
  return offset < 0.f ? -semitones : semitones;
}

}  // namespace

Loewy hardware;
GranularProcessorClouds processor;

// Sample memory and FX workspace of the granular processor, sized like the
// original Clouds firmware. Both arrays live in the Seed's internal SRAM.
uint8_t block_mem[118784];
uint8_t block_ccm[65536 - 128];

Parameters* parameters;
bool frozen = false;

void AudioCallback(AudioHandle::InputBuffer in, AudioHandle::OutputBuffer out,
                   size_t size) {
  hardware.ProcessControls();

  parameters->position =
      clamp(hardware.GetPot1() + hardware.GetCV1(), 0.f, 1.f);
  parameters->size = hardware.GetPot2();
  parameters->pitch = PitchFromPot(hardware.GetPot3());

  // Pot 4 is Clouds' density control. Its clockwise half also morphs the grain
  // envelopes from triangle to Hann (texture 0.5 to 1.0; Clouds engages its
  // diffuser above 0.75) and raises the reverb amount.
  float density = hardware.GetPot4();
  float wash = clamp((density - 0.5f) * 2.f, 0.f, 1.f);
  parameters->density = density;
  parameters->texture = 0.5f + 0.5f * wash;
  parameters->reverb = 0.2f + 0.4f * wash;

  // CV 2 freezes the buffer while high and plays one grain per rising edge.
  // The trigger flag is consumed by the processor within this block.
  float gate = hardware.GetCV2();
  bool was_frozen = frozen;
  if (gate > kFreezeOn) {
    frozen = true;
  } else if (gate < kFreezeOff) {
    frozen = false;
  }
  parameters->freeze = frozen;
  parameters->trigger = frozen && !was_frozen;
  hardware.SetLed(frozen);

  FloatFrame input[size];
  FloatFrame output[size];

  for (size_t i = 0; i < size; i++) {
    input[i].l = in[0][i];
    input[i].r = in[1][i];
    output[i].l = output[i].r = 0.f;
  }

  processor.Process(input, output, size);

  // The wet signal can exceed full scale (dense overlapping grains, the 1.2
  // post-gain, reverb and feedback) and libDaisy hard-clamps the output. The
  // original Clouds firmware soft-clips its output with this same curve
  // (SoftConvert), which the Daisy port dropped.
  for (size_t i = 0; i < size; i++) {
    out[0][i] = daisysp::SoftClip(output[i].l);
    out[1][i] = daisysp::SoftClip(output[i].r);
  }
}

int main(void) {
  Loewy::Config config;
  config.audio_block_size = 32;  // Clouds does not work with larger blocks
  hardware.Init(config);

  float sample_rate = hardware.GetSampleRate();
  InitResources(sample_rate);
  processor.Init(sample_rate, block_mem, sizeof(block_mem), block_ccm,
                 sizeof(block_ccm));
  processor.set_playback_mode(PLAYBACK_MODE_GRANULAR);
  processor.set_quality(0);  // 16-bit stereo

  parameters = processor.mutable_parameters();
  parameters->dry_wet = kDryWet;
  parameters->stereo_spread = kStereoSpread;
  parameters->feedback = kFeedback;
  parameters->freeze = false;
  parameters->trigger = false;
  parameters->gate = false;

  hardware.StartAudio(AudioCallback);

  // Prepare() does the slow, non-real-time work of the processor and has to
  // run continuously in the main loop.
  while (1) {
    processor.Prepare();
  }
}
