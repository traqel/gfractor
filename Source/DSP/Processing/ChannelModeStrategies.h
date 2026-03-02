#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include "../../Utility/ChannelMode.h"

class IChannelModeStrategy {
public:
    virtual ~IChannelModeStrategy() = default;

    virtual void prepare(const juce::dsp::ProcessSpec& spec) = 0;
    virtual void process(juce::dsp::ProcessContextReplacing<float>& context,
                         std::atomic<bool>& primaryEnabled,
                         std::atomic<bool>& secondaryEnabled,
                         juce::SmoothedValue<float>& primaryGain,
                         juce::SmoothedValue<float>& secondaryGain,
                         float fastEnvAlpha,
                         float slowEnvAlpha,
                         float& fastEnvState,
                         float& slowEnvState) = 0;
    virtual void reset() = 0;
};

class MidSideStrategy : public IChannelModeStrategy {
public:
    void prepare(const juce::dsp::ProcessSpec&) override {}

    void process(juce::dsp::ProcessContextReplacing<float>& context,
                 std::atomic<bool>& primaryEnabled,
                 std::atomic<bool>& secondaryEnabled,
                 juce::SmoothedValue<float>&,
                 juce::SmoothedValue<float>&,
                 float,
                 float,
                 float&,
                 float&) override {
        auto& block = context.getOutputBlock();
        if (block.getNumChannels() >= 2) {
            auto* leftData = block.getChannelPointer(0);
            auto* rightData = block.getChannelPointer(1);
            const bool primOn = primaryEnabled.load(std::memory_order_relaxed);
            const bool secOn = secondaryEnabled.load(std::memory_order_relaxed);

            for (size_t i = 0; i < block.getNumSamples(); ++i) {
                float mid = (leftData[i] + rightData[i]) * 0.5f;
                float side = (leftData[i] - rightData[i]) * 0.5f;

                if (!primOn) mid = 0.0f;
                if (!secOn) side = 0.0f;

                leftData[i] = mid + side;
                rightData[i] = mid - side;
            }
        }
    }

    void reset() override {}
};

class LRStereoStrategy : public IChannelModeStrategy {
public:
    void prepare(const juce::dsp::ProcessSpec&) override {}

    void process(juce::dsp::ProcessContextReplacing<float>& context,
                 std::atomic<bool>& primaryEnabled,
                 std::atomic<bool>& secondaryEnabled,
                 juce::SmoothedValue<float>&,
                 juce::SmoothedValue<float>&,
                 float,
                 float,
                 float&,
                 float&) override {
        auto& block = context.getOutputBlock();
        if (block.getNumChannels() >= 2) {
            auto* leftData = block.getChannelPointer(0);
            auto* rightData = block.getChannelPointer(1);
            const bool primOn = primaryEnabled.load(std::memory_order_relaxed);
            const bool secOn = secondaryEnabled.load(std::memory_order_relaxed);

            for (size_t i = 0; i < block.getNumSamples(); ++i) {
                if (!primOn) leftData[i] = 0.0f;
                if (!secOn) rightData[i] = 0.0f;
            }
        }
    }

    void reset() override {}
};

class TonalTransientStrategy : public IChannelModeStrategy {
public:
    void prepare(const juce::dsp::ProcessSpec&) override {}

    void process(juce::dsp::ProcessContextReplacing<float>& context,
                 std::atomic<bool>& primaryEnabled,
                 std::atomic<bool>& secondaryEnabled,
                 juce::SmoothedValue<float>& primaryGain,
                 juce::SmoothedValue<float>& secondaryGain,
                 float fastEnvAlphaParam,
                 float slowEnvAlphaParam,
                 float& envStateFast,
                 float& envStateSlow) override {
        auto& block = context.getOutputBlock();
        if (block.getNumChannels() >= 2) {
            auto* leftData = block.getChannelPointer(0);
            auto* rightData = block.getChannelPointer(1);

            primaryGain.setTargetValue(primaryEnabled.load(std::memory_order_relaxed) ? 1.0f : 0.0f);
            secondaryGain.setTargetValue(secondaryEnabled.load(std::memory_order_relaxed) ? 1.0f : 0.0f);

            for (size_t i = 0; i < block.getNumSamples(); ++i) {
                const float absMono = std::abs(leftData[i] + rightData[i]) * 0.5f;

                envStateFast += (absMono - envStateFast) * fastEnvAlphaParam;
                envStateSlow += (absMono - envStateSlow) * slowEnvAlphaParam;

                const float transientGain = envStateFast > 1e-9f
                                                ? juce::jlimit(0.0f, 1.0f, (envStateFast - envStateSlow) / envStateFast)
                                                : 0.0f;

                const float primG = primaryGain.getNextValue();
                const float secG = secondaryGain.getNextValue();
                const float gain = primG * transientGain + secG * (1.0f - transientGain);

                leftData[i] *= gain;
                rightData[i] *= gain;
            }
        }
    }

    void reset() override {}
};

class ChannelModeStrategyFactory {
public:
    static std::unique_ptr<IChannelModeStrategy> create(ChannelMode mode) {
        switch (mode) {
            case ChannelMode::MidSide:
                return std::make_unique<MidSideStrategy>();
            case ChannelMode::LR:
                return std::make_unique<LRStereoStrategy>();
            case ChannelMode::TonalTransient:
                return std::make_unique<TonalTransientStrategy>();
        }
        return std::make_unique<MidSideStrategy>();
    }
};
