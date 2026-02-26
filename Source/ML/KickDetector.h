#pragma once

#include <juce_dsp/juce_dsp.h>
#include <memory>
#include <vector>
#include <optional>

// Forward declare ONNX types if not available
#if __has_include(<onnxruntime_cxx_api.h>)
#include <onnxruntime_cxx_api.h>
#define KICK_DETECTOR_HAS_ONNX 1
#else
#define KICK_DETECTOR_HAS_ONNX 0
#endif

/**
 * KickDetector
 *
 * Loads kick_detector.onnx model and performs inference on audio data.
 * Model input: [batch_size, 39, 128] mel spectrogram
 * Model output: [batch_size, 1] probability of kick presence
 */
class KickDetector {
public:
    KickDetector();
    ~KickDetector();

    /** Initialize the ONNX model. Returns true on success. */
    bool loadModel(const juce::File &modelFile);

    /** Process audio samples and return kick probability (0-1).
     *  @param audioSamples Mono audio buffer (interleaved or mono)
     *  @param sampleRate Sample rate of the audio
     *  @return Kick probability, or std::nullopt if inference failed
     */
    std::optional<float> process(const std::vector<float> &audioSamples, double sampleRate);

    /** Check if model is loaded */
    bool isLoaded() const { return modelLoaded; }

private:
    /** Compute mel spectrogram from audio samples */
    std::vector<std::vector<float>> computeMelSpectrogram(const std::vector<float> &samples,
                                                            double sampleRate);

    /** Apply mel filterbank to magnitude spectrogram */
    std::vector<std::vector<float>> applyMelFilterbank(const std::vector<std::vector<float>> &magSpec);

#if KICK_DETECTOR_HAS_ONNX
    std::unique_ptr<Ort::Env> ortEnv;
    std::unique_ptr<Ort::Session> ortSession;
    std::unique_ptr<Ort::SessionOptions> ortSessionOptions;
    std::unique_ptr<Ort::MemoryInfo> ortMemoryInfo;
    std::vector<std::string> inputNamesStr;
    std::vector<std::string> outputNamesStr;
#endif

    bool modelLoaded = false;

    // Mel spectrogram parameters
    static constexpr int kFftSize = 512;
    static constexpr int kHopSize = 256;
    static constexpr int kNumMels = 39;
    static constexpr int kNumFrames = 128;
    static constexpr int kRequiredSamples = kHopSize * kNumFrames; // 32768 samples

    // Precomputed mel filterbank
    std::vector<std::vector<float>> melFilterbank;
    void computeMelFilterbank(double sampleRate);

    // FFT for spectrogram computation
    std::unique_ptr<juce::dsp::FFT> fft;
    std::vector<float> hannWindow;
    std::vector<float> fftBuffer;
    std::vector<float> windowedBuffer;
};
