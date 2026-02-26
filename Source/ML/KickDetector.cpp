#include "KickDetector.h"
#include <juce_core/juce_core.h>

#if __has_include("BinaryData.h")
#include "BinaryData.h"
#endif

#if KICK_DETECTOR_HAS_ONNX
#include <onnxruntime_cxx_api.h>
#endif

//==============================================================================
KickDetector::KickDetector() {
#if KICK_DETECTOR_HAS_ONNX
    ortEnv = std::make_unique<Ort::Env>(ORT_LOGGING_LEVEL_WARNING, "KickDetector");
    ortSessionOptions = std::make_unique<Ort::SessionOptions>();
    ortSessionOptions->SetIntraOpNumThreads(1);
    ortSessionOptions->SetGraphOptimizationLevel(GraphOptimizationLevel::ORT_ENABLE_EXTENDED);
    ortMemoryInfo = std::make_unique<Ort::MemoryInfo>(Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault));
#endif

    // Initialize FFT
    fft = std::make_unique<juce::dsp::FFT>(9); // 512-point FFT

    // Precompute Hann window
    hannWindow.resize(kFftSize);
    for (int i = 0; i < kFftSize; ++i) {
        hannWindow[i] = 0.5f * (1.0f - std::cos(2.0f * juce::float_Pi * i / (kFftSize - 1)));
    }

    fftBuffer.resize(kFftSize * 2);
    windowedBuffer.resize(kFftSize);
}

KickDetector::~KickDetector() = default;

//==============================================================================
bool KickDetector::loadModel(const juce::File &modelFile) {
#if KICK_DETECTOR_HAS_ONNX
    try {
        // Try to load from BinaryData first
#if __has_include("BinaryData.h")
        int dataSize = 0;
        const char* modelData = BinaryData::getNamedResource("kick_detector_onnx", dataSize);

        if (modelData != nullptr && dataSize > 0) {
            juce::Logger::writeToLog("KickDetector: Loading model from BinaryData (" + juce::String(dataSize) + " bytes)");

            // Create session from memory
            Ort::SessionOptions sessionOptions;
            sessionOptions.SetIntraOpNumThreads(1);
            sessionOptions.SetGraphOptimizationLevel(GraphOptimizationLevel::ORT_ENABLE_EXTENDED);

            ortSession = std::make_unique<Ort::Session>(*ortEnv,
                modelData,
                static_cast<size_t>(dataSize),
                sessionOptions);
        } else
#endif
        {
            // Fall back to file loading
            if (!modelFile.existsAsFile()) {
                juce::Logger::writeToLog("KickDetector: Model file not found: " + modelFile.getFullPathName());
                return false;
            }

            ortSession = std::make_unique<Ort::Session>(*ortEnv,
                modelFile.getFullPathName().toUTF8(),
                *ortSessionOptions);
        }

        // Get input/output names
        size_t numInputNodes = ortSession->GetInputCount();
        size_t numOutputNodes = ortSession->GetOutputCount();

        if (numInputNodes == 0 || numOutputNodes == 0) {
            juce::Logger::writeToLog("KickDetector: Model has no input/output nodes");
            return false;
        }

        juce::Logger::writeToLog("KickDetector: " + juce::String(numInputNodes) + " inputs, " + juce::String(numOutputNodes) + " outputs");

        Ort::AllocatorWithDefaultOptions allocator;

        // Input name
        auto inputName = ortSession->GetInputNameAllocated(0, allocator);
        inputNamesStr.push_back(inputName.get());
        juce::Logger::writeToLog("KickDetector: Input name: " + juce::String(inputName.get()));

        // Output name
        auto outputName = ortSession->GetOutputNameAllocated(0, allocator);
        outputNamesStr.push_back(outputName.get());
        juce::Logger::writeToLog("KickDetector: Output name: " + juce::String(outputName.get()));

        modelLoaded = true;
        juce::Logger::writeToLog("KickDetector: Model loaded successfully");
        return true;
    } catch (const Ort::Exception &e) {
        juce::Logger::writeToLog("KickDetector: ONNX Error: " + juce::String(e.what()));
        return false;
    }
#else
    juce::Logger::writeToLog("KickDetector: ONNX Runtime not available");
    (void)modelFile;
    return false;
#endif
}

//==============================================================================
std::optional<float> KickDetector::process(const std::vector<float> &audioSamples, double sampleRate) {
    if (!modelLoaded) {
        return std::nullopt;
    }

    // Compute mel spectrogram
    auto melSpec = computeMelSpectrogram(audioSamples, sampleRate);
    if (melSpec.empty()) {
        return std::nullopt;
    }

// Temporarily disabled - ONNX Runtime crashing
#if 0 && KICK_DETECTOR_HAS_ONNX
    try {
        // Prepare input tensor: [1, 39, 128]
        std::vector<float> inputData(kNumMels * kNumFrames);
        for (int f = 0; f < kNumFrames; ++f) {
            for (int m = 0; m < kNumMels; ++m) {
                // Transpose: melSpec[f][m] -> input[m][f]
                inputData[m * kNumFrames + f] = melSpec[f][m];
            }
        }

        // Create input tensor
        std::vector<int64_t> inputShape = {1, kNumMels, kNumFrames};
        auto inputTensor = Ort::Value::CreateTensor<float>(
            *ortMemoryInfo,
            inputData.data(),
            inputData.size(),
            inputShape.data(),
            inputShape.size()
        );

        // Run inference with named inputs/outputs
        std::vector<const char*> inputNamesPtr(1, inputNamesStr[0].c_str());
        std::vector<const char*> outputNamesPtr(1, outputNamesStr[0].c_str());

        auto outputTensor = ortSession->Run(
            Ort::RunOptions{nullptr},
            inputNamesPtr.data(),
            &inputTensor,
            1,
            outputNamesPtr.data(),
            1
        );

        // Get the output
        float *outputData = outputTensor.front().GetTensorMutableData<float>();
        float probability = outputData[0];

        return probability;
    } catch (const Ort::Exception &e) {
        juce::Logger::writeToLog("KickDetector: Inference error: " + juce::String(e.what()));
        return std::nullopt;
    }
#else
    // Return random for testing
    return 0.5f + (rand() % 100) / 200.0f;
#endif
}

//==============================================================================
std::vector<std::vector<float>> KickDetector::computeMelSpectrogram(const std::vector<float> &samples,
                                                                      double sampleRate) {
    // Ensure we have enough samples
    if (samples.size() < kRequiredSamples) {
        return {};
    }

    // Check if audio is loud enough
    float maxS = 0.0f;
    for (size_t i = samples.size() - 1000; i < samples.size(); ++i) {
        maxS = std::max(maxS, std::abs(samples[i]));
    }
    if (maxS < 0.01f) {
        // Audio too quiet, return zeros
        return std::vector<std::vector<float>>(kNumFrames, std::vector<float>(kNumMels, 0.0f));
    }

    // Use the most recent kRequiredSamples
    size_t startIdx = samples.size() - kRequiredSamples;

    // Initialize mel filterbank if needed
    if (melFilterbank.empty()) {
        computeMelFilterbank(sampleRate);
    }

    std::vector<std::vector<float>> melSpec(kNumFrames, std::vector<float>(kNumMels, 0.0f));

    // Compute STFT and mel spectrogram for each frame
    for (int frame = 0; frame < kNumFrames; ++frame) {
        int frameStart = static_cast<int>(startIdx) + frame * kHopSize;

        // Copy and window
        for (int i = 0; i < kFftSize; ++i) {
            if (frameStart + i < static_cast<int>(samples.size())) {
                windowedBuffer[i] = samples[frameStart + i] * hannWindow[i];
            } else {
                windowedBuffer[i] = 0.0f;
            }
        }

        // FFT
        fft->performRealOnlyForwardTransform(windowedBuffer.data(), fftBuffer.data());

        // Compute magnitude spectrum
        std::vector<float> magSpec(kFftSize / 2 + 1);
        for (int i = 0; i < kFftSize / 2 + 1; ++i) {
            float real = fftBuffer[i * 2];
            float imag = fftBuffer[i * 2 + 1];
            magSpec[i] = std::sqrt(real * real + imag * imag) + 1e-10f;
        }

        // Apply mel filterbank
        for (int m = 0; m < kNumMels; ++m) {
            float melEnergy = 0.0f;
            for (int i = 0; i < static_cast<int>(magSpec.size()); ++i) {
                melEnergy += magSpec[i] * melFilterbank[m][i];
            }
            // Convert to dB
            melSpec[frame][m] = 10.0f * std::log10(melEnergy + 1e-10f);
            // Normalize to roughly 0-1 range (typical dB range is -80 to 0)
            melSpec[frame][m] = (melSpec[frame][m] + 80.0f) / 80.0f;
            melSpec[frame][m] = juce::jlimit(0.0f, 1.0f, melSpec[frame][m]);
        }
    }

    return melSpec;
}

//==============================================================================
void KickDetector::computeMelFilterbank(double sampleRate) {
    melFilterbank.resize(kNumMels, std::vector<float>(kFftSize / 2 + 1, 0.0f));

    // Compute mel frequencies
    auto hzToMel = [](float hz) {
        return 2595.0f * std::log10(1.0f + hz / 700.0f);
    };

    auto melToHz = [](float mel) {
        return 700.0f * (std::pow(10.0f, mel / 2595.0f) - 1.0f);
    };

    float minMel = hzToMel(0.0f);
    float maxMel = hzToMel(sampleRate / 2.0f);

    // Generate mel band centers
    std::vector<float> melCenters(kNumMels + 2);
    for (int i = 0; i < kNumMels + 2; ++i) {
        melCenters[i] = minMel + (maxMel - minMel) * i / (kNumMels + 1);
    }

    // Convert to Hz
    std::vector<float> hzCenters(kNumMels + 2);
    for (int i = 0; i < kNumMels + 2; ++i) {
        hzCenters[i] = melToHz(melCenters[i]);
    }

    // Convert to FFT bin indices
    std::vector<int> binIndices(kNumMels + 2);
    for (int i = 0; i < kNumMels + 2; ++i) {
        binIndices[i] = static_cast<int>(std::floor(hzCenters[i] * kFftSize / sampleRate));
    }

    // Create triangular filterbank
    for (int m = 0; m < kNumMels; ++m) {
        for (int i = binIndices[m]; i < binIndices[m + 1]; ++i) {
            if (i >= 0 && i <= kFftSize / 2) {
                melFilterbank[m][i] = static_cast<float>(i - binIndices[m]) / (binIndices[m + 1] - binIndices[m]);
            }
        }
        for (int i = binIndices[m + 1]; i <= binIndices[m + 2]; ++i) {
            if (i >= 0 && i <= kFftSize / 2) {
                melFilterbank[m][i] = static_cast<float>(binIndices[m + 2] - i) / (binIndices[m + 2] - binIndices[m + 1]);
            }
        }
    }
}
