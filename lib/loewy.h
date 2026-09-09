#pragma once

#include <stddef.h>
#include <stdint.h>

#include "daisy_seed.h"

namespace loewy {

/**
 * Hardware abstraction for the Löwenzahnhonig Eurorack module: a Daisy Seed
 * with four potentiometers (P1-P4 on A0-A3) and two CV inputs (CV1-CV2 on
 * A4-A5).
 *
 * The controls are read through libDaisy's AnalogControl, which smooths the
 * ADC readings with a one-pole filter. The filter coefficient is derived from
 * the audio callback rate, so ProcessControls() has to be called exactly once
 * per audio callback:
 *
 *   loewy::Loewy hardware;
 *
 *   void AudioCallback(daisy::AudioHandle::InputBuffer in,
 *                      daisy::AudioHandle::OutputBuffer out, size_t size) {
 *     hardware.ProcessControls();
 *     float gain = hardware.GetPot1();
 *     ...
 *   }
 *
 *   int main(void) {
 *     hardware.Init();
 *     hardware.StartAudio(AudioCallback);
 *     while (1) {}
 *   }
 *
 * GetPot() and GetCV() return the last processed value without touching the
 * hardware, so they can also be called from the main loop.
 */
class Loewy {
 public:
  enum class Pot : uint8_t { POT_1, POT_2, POT_3, POT_4 };
  enum class CV : uint8_t { CV_1, CV_2 };

  static constexpr size_t kNumPots = 4;
  static constexpr size_t kNumCVs = 2;

  struct Config {
    // Samples per channel handled by each audio callback (libDaisy default).
    size_t audio_block_size = 48;

    // Slew time of the control smoothing filters, in seconds. Smoothing only
    // takes effect when the slew time is longer than two audio callbacks, so
    // small block sizes get more smoothing out of the same setting.
    float pot_slew_seconds = 0.002f;
    float cv_slew_seconds = 0.0005f;

    // The CV input stage of the module is inverting: 0 V reads as full scale.
    // Flipping the reading (1 - x) makes 0 V read as 0.0, like the pots. This
    // matches the `1 - GetFloat()` that the firmwares used before this class.
    bool flip_cv = true;
  };

  // Initializes the Daisy Seed, the audio block size and the ADC channels of
  // the pots and CV inputs.
  void Init(const Config& config);
  void Init();  // Same as Init(Config()).

  // Updates the smoothing filters of all controls with the latest ADC
  // readings. Call once per audio callback (see class documentation).
  void ProcessControls();

  // Last processed value of a potentiometer, 0.0 (fully CCW) to 1.0.
  float GetPot(Pot pot) const {
    return pots_[static_cast<size_t>(pot)].Value();
  }

  // Last processed value of a CV input, 0.0 (0 V) to 1.0.
  float GetCV(CV cv) const { return cvs_[static_cast<size_t>(cv)].Value(); }

  float GetPot1() const { return GetPot(Pot::POT_1); }
  float GetPot2() const { return GetPot(Pot::POT_2); }
  float GetPot3() const { return GetPot(Pot::POT_3); }
  float GetPot4() const { return GetPot(Pot::POT_4); }

  float GetCV1() const { return GetCV(CV::CV_1); }
  float GetCV2() const { return GetCV(CV::CV_2); }

  // Audio sample rate (Hz) and block size as configured on the Daisy Seed.
  float GetSampleRate() { return hw_.AudioSampleRate(); }
  size_t GetBlockSize() { return hw_.AudioBlockSize(); }

  // Starts the audio engine with the given callback.
  void StartAudio(daisy::AudioHandle::AudioCallback callback) {
    hw_.StartAudio(callback);
  }

  // Sets the on-board LED of the Daisy Seed.
  void SetLed(bool on) { hw_.SetLed(on); }

  // Direct access to the Daisy Seed for anything not covered above.
  daisy::DaisySeed& GetHardware() { return hw_; }

 private:
  void InitAdc();
  void InitControls();

  daisy::DaisySeed hw_;
  daisy::AnalogControl pots_[kNumPots];
  daisy::AnalogControl cvs_[kNumCVs];
  Config config_;
};

}  // namespace loewy
