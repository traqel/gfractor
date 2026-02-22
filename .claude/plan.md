# SOLID Refactoring Plan — SpectrumAnalyzer

## Current State

The refactoring is partially done. Key progress so far:
- **5 interfaces** extracted: `IAudioDataSink`, `IGhostDataSink`, `IPeakLevelSource`, `ISpectrumControls`, `ISpectrumDisplaySettings`
- **`ChannelMode`** enum + `ChannelDecoder` extracted to shared utility
- **`FFTProcessor`** class extracted (has own files) — but **NOT YET used** by SpectrumAnalyzer (duplicate code remains)
- **`AudioRingBuffer`** class extracted (has own files) — but **NOT YET used** by AudioVisualizerBase or GhostSpectrum (duplicate FIFO code remains)
- **`FooterBar`** decoupled — takes `ISpectrumControls&` + `IPeakLevelSource&` instead of concrete `SpectrumAnalyzer&`
- **`PreferencePanel`** decoupled — takes `ISpectrumDisplaySettings&`
- **`AnalyzerSettings`** decoupled — uses `ISpectrumDisplaySettings&` for save/load

## Remaining SOLID Violations

### 1. SpectrumAnalyzer is still a god class (~377 lines header, ~692 lines impl)
It still directly owns: FFT engine, Hann window, smoothing ranges, fft work buffers, instant dB arrays, temporal smoothing — all **duplicated** in `FFTProcessor` which was extracted but never integrated.

### 2. AudioVisualizerBase duplicates AudioRingBuffer
`AudioVisualizerBase` has its own FIFO + rolling buffer implementation (fifo, fifoL, fifoR, rollingL, rollingR, writePos, rollingSize, drain, resetFifo, resizeRolling). `AudioRingBuffer` is an exact extraction of this — but not composed yet.

### 3. GhostSpectrum duplicates AudioRingBuffer
`GhostSpectrum` has its own FIFO + rolling buffer (fifo, bufferL, bufferR, rollingL, rollingR, writePos). Same duplication.

### 4. FooterBar still takes `gFractorAudioProcessor&` directly
It needs `processorRef.getAPVTS()` for pill button parameter binding and `processorRef.setLRMode()`. This concrete dependency could be narrowed.

## Plan

### Step 1: Compose `FFTProcessor` into `SpectrumAnalyzer` (SRP — extract DSP from UI)
- Add `FFTProcessor fftProcessor;` member to `SpectrumAnalyzer`
- Remove all duplicate FFT members from SpectrumAnalyzer: `forwardFFT`, `hannWindow`, `fftDataMid`, `fftDataSide`, `instantMidDb`, `instantSideDb`, `smoothingRanges`, `smoothingTemp`, `smoothingPrefix`, `temporalDecay`
- Remove duplicate methods: `processFFTBlock()`, `applyOctaveSmoothing()`, `precomputeSmoothingRanges()`
- Forward `setFftOrder`, `setSmoothing`, `setSlope`, `setChannelMode` to `fftProcessor`
- Update `processDrainedData()` to call `fftProcessor.processBlock()` instead of `processFFTBlock()`
- Update `GhostSpectrum::processDrained()` lambda to use the shared `fftProcessor`

### Step 2: Compose `AudioRingBuffer` into `AudioVisualizerBase` (DRY)
- Replace the hand-rolled FIFO + rolling buffer in `AudioVisualizerBase` with an `AudioRingBuffer` member
- Forward `pushStereoData()` → `ringBuffer.push()`
- Forward `drainFifo()` → `ringBuffer.drain()`
- Forward `resizeRollingBuffer()` → `ringBuffer.resizeRolling()`
- Forward `resetFifo()` → `ringBuffer.resetFifo()`
- Update protected accessors (`getRollingL/R`, `getRollingWritePos`) to delegate to `ringBuffer`

### Step 3: Compose `AudioRingBuffer` into `GhostSpectrum` (DRY)
- Replace GhostSpectrum's hand-rolled FIFO (fifo, bufferL, bufferR, rollingL, rollingR, writePos) with `AudioRingBuffer ringBuffer`
- Forward `pushData()` → `ringBuffer.push()`
- Forward `drainSilently()` → `ringBuffer.drainSilently()`
- Update `processDrained()` to use `ringBuffer.drain()`, `ringBuffer.getL/R()`, `ringBuffer.getWritePos()`
- Forward `resetFifo()` → `ringBuffer.resetFifo()`
- Update `resetBuffers()` → `ringBuffer.resizeRolling()`

### Step 4: Build and verify
- Run CMake configure + build to ensure compilation
- Verify no regressions in functionality
