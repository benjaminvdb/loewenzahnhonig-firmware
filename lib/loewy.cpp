#include "loewy.h"

namespace loewy {

namespace {

constexpr size_t kNumAdcChannels = Loewy::kNumPots + Loewy::kNumCVs;

// Daisy Seed pins of the ADC channels, in channel order: P1-P4 on A0-A3
// (pins 15-18) followed by CV1-CV2 on A4-A5 (pins 19-20).
constexpr uint8_t kAdcPins[kNumAdcChannels] = {15, 16, 17, 18, 19, 20};

}  // namespace

void Loewy::Init() { Init(Config()); }

void Loewy::Init(const Config& config) {
  config_ = config;

  hw_.Init();
  hw_.SetAudioBlockSize(config_.audio_block_size);

  InitAdc();
  InitControls();
}

void Loewy::InitAdc() {
  daisy::AdcChannelConfig adc_config[kNumAdcChannels];
  for (size_t i = 0; i < kNumAdcChannels; i++) {
    adc_config[i].InitSingle(daisy::DaisySeed::GetPin(kAdcPins[i]));
  }
  hw_.adc.Init(adc_config, kNumAdcChannels);
  hw_.adc.Start();
}

void Loewy::InitControls() {
  // The controls are processed once per audio callback, so the smoothing
  // filters run at the callback rate, not at the audio sample rate.
  const float rate = hw_.AudioCallbackRate();

  for (size_t i = 0; i < kNumPots; i++) {
    pots_[i].Init(hw_.adc.GetPtr(i), rate, /*flip=*/false, /*invert=*/false,
                  config_.pot_slew_seconds);
  }
  for (size_t i = 0; i < kNumCVs; i++) {
    // `flip` maps the reading to 1 - x; `invert` would negate it instead.
    cvs_[i].Init(hw_.adc.GetPtr(kNumPots + i), rate, /*flip=*/config_.flip_cv,
                 /*invert=*/false, config_.cv_slew_seconds);
  }
}

void Loewy::ProcessControls() {
  for (auto& pot : pots_) pot.Process();
  for (auto& cv : cvs_) cv.Process();
}

}  // namespace loewy
