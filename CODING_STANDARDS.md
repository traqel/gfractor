# C++ Coding Standards for gFractor JUCE Plugin Project

**Version:** 1.0
**Last Updated:** 2026-03-02
**Applies To:** All C++ code in the gFractor plugin project

---

## Table of Contents

1. [Code Style & Formatting](#1-code-style--formatting)
2. [JUCE-Specific Standards](#2-juce-specific-standards)
3. [Realtime Safety Rules](#3-realtime-safety-rules)
4. [Modern C++ Best Practices](#4-modern-c-best-practices)
5. [SOLID & GRASP Design Principles](#5-solid--grasp-design-principles)
6. [Project-Specific Conventions](#6-project-specific-conventions)
7. [Error Handling](#7-error-handling)
8. [Documentation Standards](#8-documentation-standards)
9. [Testing Standards](#9-testing-standards)

---

## 1. Code Style & Formatting

### 1.1 Naming Conventions

**Classes and Structs** - PascalCase
```cpp
// Good
class gFractorAudioProcessor;
class WaveformVisualizer;
struct PerformanceMetrics;

// Bad
class gFractorAudioProcessor;  // Wrong case
class waveform_visualizer;         // Snake case
```

**Functions and Methods** - camelCase
```cpp
// Good
void processBlock (juce::AudioBuffer<float>& buffer);
void setWaveformColour (juce::Colour colour);
bool isBusesLayoutSupported (const BusesLayout& layouts) const;

// Bad
void ProcessBlock (...);  // PascalCase for methods
void set_waveform_colour (...);  // Snake case
```

**Variables** - camelCase
```cpp
// Good
int sampleRate;
float gainValue;
bool isPrepared;

// Bad
int SampleRate;     // PascalCase
float gain_value;   // Snake case
bool is_prepared;   // Snake case
```

**Member Variables** - camelCase (no prefix)
```cpp
// Good
class MyClass
{
private:
    int sampleCount;
    bool bypassed;
    juce::AudioBuffer<float> buffer;
};

// Acceptable (for disambiguation)
class MyClass
{
private:
    int sampleCount = 0;          // Default initialization preferred
    std::atomic<bool> bypassed;   // Clear type makes prefix unnecessary
};

// Bad - Hungarian notation
class MyClass
{
private:
    int m_sampleCount;    // Avoid m_ prefix
    bool bBypassed;       // Avoid type prefixes
    float fGain;          // Avoid type prefixes
};
```

**Constants** - camelCase or PascalCase (be consistent)
```cpp
// Good - namespace constants
namespace ParameterIDs
{
    inline constexpr auto gain = "gain";
    inline constexpr auto dryWet = "dryWet";
    inline constexpr int parameterVersion = 1;
}

// Good - class constants
class MyClass
{
    static constexpr int MaxChannels = 8;
    static constexpr double DefaultSampleRate = 44100.0;
};

// Bad
#define MAX_CHANNELS 8  // Avoid preprocessor macros for constants
const int max_channels = 8;  // Snake case
```

**Namespaces** - PascalCase (for project namespaces) or lowercase (for small utility namespaces)
```cpp
// Good
namespace ParameterIDs { ... }
namespace ParameterDefaults { ... }

// Bad
namespace PARAMETERIDS { ... }   // All caps
namespace parameter_ids { ... }  // Snake case (unless utility namespace)
```

**Enum Classes** - PascalCase for type, PascalCase for values
```cpp
// Good
enum class ProcessingState
{
    Inactive,
    Active,
    Bypassed
};

// Bad
enum class processing_state { INACTIVE, ACTIVE };  // Inconsistent
```

### 1.2 Indentation and Spacing

**Indentation:** 4 spaces (no tabs)
```cpp
// Good
void processBlock (juce::AudioBuffer<float>& buffer)
{
    if (bypassed)
        return;

    for (int i = 0; i < buffer.getNumSamples(); ++i)
    {
        // Process sample
    }
}

// Bad - uses tabs or inconsistent spacing
void processBlock (juce::AudioBuffer<float>& buffer)
{
→   if (bypassed)  // Tab character
  →   return;      // Inconsistent indentation
}
```

**Spacing Around Operators**
```cpp
// Good
int result = a + b;
float gain = juce::Decibels::decibelsToGain (6.0f);
if (x == y && z != w)

// Bad
int result=a+b;              // No spaces
float gain = decibels (6.0f);// Space before (
if (x==y&&z!=w)              // No spaces around operators
```

**Function Parameters - Space After Opening Parenthesis (JUCE Style)**
```cpp
// Good - JUCE convention (space after opening paren)
void myFunction (int param1, float param2);
if (condition)
for (int i = 0; i < count; ++i)

// Also acceptable - standard C++ style (no space)
void myFunction(int param1, float param2);
if(condition)

// Choose one style and be consistent within the project
// This project uses JUCE style (space after opening paren)
```

### 1.3 Brace Placement

**Opening Brace on Same Line for Control Structures**
```cpp
// Good
if (condition)
{
    // code
}
else
{
    // code
}

for (int i = 0; i < count; ++i)
{
    // code
}

while (running)
{
    // code
}

// Bad - K&R style (inconsistent with codebase)
if (condition) {
    // code
} else {
    // code
}
```

**Opening Brace on New Line for Functions and Classes**
```cpp
// Good
class MyClass
{
public:
    void myMethod()
    {
        // code
    }
};

// Acceptable for inline one-liners
class MyClass
{
public:
    int getValue() const { return value; }
private:
    int value = 0;
};
```

**Single-Line Conditionals - Braces Optional but Preferred**
```cpp
// Good - explicit braces
if (bypassed)
    return;

if (condition)
{
    doSomething();
}

// Acceptable for very short statements
if (bypassed) return;

// Bad - confusing multi-line without braces
if (condition)
    doSomething();
    doSomethingElse();  // Not part of if! Misleading indentation
```

### 1.4 Line Length Limits

**Maximum Line Length:** 100-120 characters (soft limit)

```cpp
// Good - reasonable line length
void setWaveformVisualizer (WaveformVisualizer* visualizer);

// Good - break long lines for readability
juce::AudioProcessorValueTreeState apvts (*this,
                                           nullptr,
                                           "Parameters",
                                           ParameterLayout::createParameterLayout());

// Bad - excessively long line
auto result = calculateSomethingVeryComplex (parameter1, parameter2, parameter3, parameter4, parameter5, parameter6) * someOtherVeryLongCalculation();

// Better
auto result = calculateSomethingVeryComplex (parameter1, parameter2, parameter3,
                                              parameter4, parameter5, parameter6)
            * someOtherVeryLongCalculation();
```

### 1.5 Comment Style

**Single-Line Comments** - Use `//` for inline and short comments
```cpp
// Good
int sampleCount = 0;  // Initialize to zero

// Clear internal state of all processors
gainProcessor.reset();

// Bad
/* Avoid block comments for single lines */
int sampleCount = 0;  /* Initialize to zero */
```

**Multi-Line Comments** - Use `//` for consistency or `/* */` for large blocks
```cpp
// Good - consistent style
// This is a multi-line comment explaining
// a complex algorithm or design decision.
// Use this style for most multi-line comments.

// Also acceptable for large documentation blocks
/**
 * This is a documentation comment for a class or function.
 * It uses Doxygen-style formatting.
 */

// Bad - inconsistent mixing
/* This comment uses block style
   but is mixed with // comments */
// which makes it inconsistent
```

**Section Separators** - Use JUCE-style separators
```cpp
// Good - JUCE convention
class MyClass
{
public:
    //==============================================================================
    // Section title
    void publicMethod();

private:
    //==============================================================================
    // Another section
    int memberVariable;
};

// Bad - inconsistent separators
//-------------------------------------
//***********************************
//////////////////////////////////////////////
```

---

## 2. JUCE-Specific Standards

### 2.1 JUCE Naming Conventions

Follow JUCE framework conventions for consistency:

```cpp
// Good - JUCE style
void paint (juce::Graphics& g) override;
void resized() override;
void timerCallback() override;
void buttonClicked (juce::Button* button);

// Bad - deviating from JUCE conventions
void Paint (juce::Graphics& g) override;      // Wrong case
void on_resize() override;                     // Wrong name and style
```

### 2.2 When to Use JUCE Types vs std Types

**Prefer JUCE types for:**
- Strings: `juce::String` (better for UI, conversions, and audio parameter text)
- Arrays: `juce::Array<T>` or `juce::OwnedArray<T>` (when you need JUCE-specific features)
- Files: `juce::File` (cross-platform path handling)
- Atomics: `std::atomic<T>` is fine, but JUCE's are also available
- Math: `juce::jmin`, `juce::jmax`, `juce::jlimit` (inline, optimized)

**Prefer std types for:**
- Smart pointers: `std::unique_ptr`, `std::shared_ptr`, `std::weak_ptr`
- Containers: `std::vector`, `std::array`, `std::map` (unless JUCE-specific features needed)
- Algorithms: `std` algorithms where appropriate
- Type traits and utilities: `std::optional`, `std::variant`

```cpp
// Good - appropriate type usage
class MyComponent : public juce::Component
{
public:
    void setTitle (const juce::String& title)  // JUCE String for UI
    {
        titleText = title;
    }

private:
    juce::String titleText;
    std::vector<float> audioData;      // std::vector for raw data
    std::unique_ptr<Filter> filter;    // std smart pointer
    juce::OwnedArray<Component> children;  // JUCE for component ownership
};

// Bad - inconsistent or inappropriate usage
void setTitle (const std::string& title)  // std::string needs conversion
{
    titleText = juce::String (title);  // Unnecessary conversion
}
```

### 2.3 Component Lifecycle Best Practices

**Constructor** - Setup, don't allocate heavy resources
```cpp
// Good
MyComponent::MyComponent()
{
    addAndMakeVisible (slider);
    slider.setRange (0.0, 1.0);
    setSize (400, 300);
}

// Bad - heavy work in constructor
MyComponent::MyComponent()
{
    loadLargeImageFromDisk();  // Do in componentDidMount or async
    performComplexCalculation();  // Too heavy for constructor
}
```

**Destructor** - Clean up resources, unregister listeners
```cpp
// Good
MyComponent::~MyComponent()
{
    setLookAndFeel (nullptr);  // Always clear custom LookAndFeel
    audioProcessor.setWaveformVisualizer (nullptr);  // Clear raw pointers
}
```

**resized()** - Layout only, no painting
```cpp
// Good
void MyComponent::resized()
{
    auto bounds = getLocalBounds();
    slider.setBounds (bounds.removeFromTop (50));
    label.setBounds (bounds);
}

// Bad
void MyComponent::resized()
{
    // Don't do painting/graphics work here
    graphics.fillAll (juce::Colours::black);  // Wrong! Use paint()
}
```

### 2.4 LookAndFeel Usage Guidelines

**Always set and clear LookAndFeel properly**
```cpp
// Good
class MyEditor : public juce::AudioProcessorEditor
{
public:
    MyEditor()
    {
        setLookAndFeel (&customLookAndFeel);  // Set in constructor
    }

    ~MyEditor() override
    {
        setLookAndFeel (nullptr);  // CRITICAL: Clear in destructor
    }

private:
    CustomLookAndFeel customLookAndFeel;  // Owned by editor
};

// Bad - memory leak and undefined behavior
class MyEditor : public juce::AudioProcessorEditor
{
public:
    ~MyEditor() override
    {
        // Missing setLookAndFeel(nullptr)!
        // Component may try to use deleted LookAndFeel
    }
};
```

**LookAndFeel ownership**
```cpp
// Good - owned as member
class MyEditor : public juce::AudioProcessorEditor
{
private:
    CustomLookAndFeel lookAndFeel;  // Lifetime managed by editor
};

// Good - shared LookAndFeel
class MyPlugin
{
public:
    juce::SharedResourcePointer<CustomLookAndFeel> lookAndFeel;
};

// Bad - dangling pointer
void MyComponent::setup()
{
    CustomLookAndFeel laf;  // Local variable!
    setLookAndFeel (&laf);  // Pointer becomes invalid when function returns
}
```

---

## 3. Realtime Safety Rules

### 3.1 What's Forbidden in Audio Thread (`processBlock`)

**NEVER do these in `processBlock()` or any code called from it:**

1. **Memory Allocation**
```cpp
// BAD - FORBIDDEN in processBlock()
void processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    std::vector<float> temp;  // BAD: Default constructor allocates
    temp.resize (512);         // BAD: Allocation

    auto ptr = new float[512]; // BAD: Dynamic allocation
    delete[] ptr;              // BAD: Deallocation

    juce::String msg = "Processing";  // BAD: May allocate

    // BAD: May allocate (only safe if preallocated)
    midiBuffer.addEvent (juce::MidiMessage::noteOn (1, 60, 1.0f), 0);
}
```

2. **Locks and Mutexes**
```cpp
// BAD - FORBIDDEN in processBlock()
void processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    std::lock_guard<std::mutex> lock (processorMutex);  // BAD: Blocking
    juce::ScopedLock sl (criticalSection);              // BAD: Blocking

    // BAD: Atomic compare-exchange loops can block
    while (!atomicFlag.compare_exchange_weak (expected, desired)) {}
}
```

3. **File I/O and System Calls**
```cpp
// BAD - FORBIDDEN in processBlock()
void processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::File outputFile ("output.wav");       // BAD: File operations
    outputFile.create();                         // BAD: Disk I/O

    juce::Logger::writeToLog ("Processing");    // BAD: May allocate/write to disk

    std::cout << "Debug info";                  // BAD: I/O operation

    juce::Thread::sleep (10);                   // BAD: Blocking call
}
```

4. **Waiting or Blocking Operations**
```cpp
// BAD - FORBIDDEN in processBlock()
void processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    while (someCondition) {}  // BAD: Busy waiting (unless lock-free and bounded)

    semaphore.wait();         // BAD: Blocking

    future.get();             // BAD: Blocking until ready
}
```

### 3.2 Allowed Operations in Audio Thread

**Safe operations:**

```cpp
// GOOD - Safe in processBlock()
void processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;  // OK: Just sets CPU flags

    // OK: Reading/writing to pre-allocated buffers
    auto* channelData = buffer.getWritePointer (0);
    for (int i = 0; i < buffer.getNumSamples(); ++i)
        channelData[i] *= gainValue;

    // OK: Lock-free FIFO operations
    int numReady = fifo.getNumReady();
    if (numReady > 0)
        fifo.read (outputBuffer, numReady);

    // OK: Atomic reads/writes (non-blocking)
    float currentGain = gainSmoothed.getNextValue();
    bool isBypassed = bypassed.load (std::memory_order_relaxed);

    // OK: Pure calculations (no allocations)
    float phase += phaseIncrement;
    float output = std::sin (phase);

    // OK: jassert (removed in release builds)
    jassert (buffer.getNumSamples() <= maxBlockSize);
}
```

### 3.3 Memory Allocation Guidelines

**Pre-allocate in `prepareToPlay()`**

```cpp
// Good - allocation happens in prepareToPlay()
class MyProcessor : public juce::AudioProcessor
{
public:
    void prepareToPlay (double sampleRate, int samplesPerBlock) override
    {
        // Allocate all needed memory here
        tempBuffer.setSize (2, samplesPerBlock);
        delayBuffer.setSize (2, static_cast<int> (sampleRate * 2.0));  // 2 sec delay

        // Initialize DSP components (may allocate internally)
        dsp::ProcessSpec spec { sampleRate,
                                static_cast<juce::uint32> (samplesPerBlock),
                                2 };
        filter.prepare (spec);

        // Reserve vector capacity (prevents reallocation)
        midiEvents.reserve (128);
    }

    void processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi) override
    {
        // Use pre-allocated buffers - no allocation here
        tempBuffer.copyFrom (0, 0, buffer, 0, 0, buffer.getNumSamples());

        // OK: Uses preallocated capacity
        if (midiEvents.size() < midiEvents.capacity())
            midiEvents.push_back (event);  // Safe if capacity available
    }

private:
    juce::AudioBuffer<float> tempBuffer;
    juce::AudioBuffer<float> delayBuffer;
    std::vector<MidiEvent> midiEvents;
    juce::dsp::ProcessorChain<...> filter;
};
```

### 3.4 Lock-Free Programming Patterns

**Use JUCE's AbstractFifo for inter-thread communication**

```cpp
// Good - lock-free audio data transfer
class WaveformVisualizer : public juce::Component
{
public:
    // Called from audio thread (realtime-safe)
    void pushAudioData (const juce::AudioBuffer<float>& buffer)
    {
        const int numSamples = buffer.getNumSamples();

        int start1, size1, start2, size2;
        fifo.prepareToWrite (numSamples, start1, size1, start2, size2);

        if (size1 > 0)
            fifoBuffer.copyFrom (start1, buffer.getReadPointer (0), size1);
        if (size2 > 0)
            fifoBuffer.copyFrom (start2, buffer.getReadPointer (0) + size1, size2);

        fifo.finishedWrite (size1 + size2);
    }

    // Called from message thread
    void timerCallback() override
    {
        int numReady = fifo.getNumReady();
        if (numReady > 0)
        {
            // Read from FIFO (lock-free)
            // ... process data for UI
        }
    }

private:
    static constexpr int fifoSize = 8192;
    juce::AbstractFifo fifo { fifoSize };
    std::array<float, fifoSize> fifoBuffer;
};
```

**Use std::atomic for simple flags and values**

```cpp
// Good - atomic operations for thread communication
class MyDSP
{
public:
    // Called from message thread
    void setGain (float newGain)
    {
        gainSmoothed.setTargetValue (newGain);  // Thread-safe (SmoothedValue)
    }

    void setBypassed (bool shouldBypass)
    {
        bypassed.store (shouldBypass, std::memory_order_release);  // Atomic write
    }

    // Called from audio thread
    void process (juce::AudioBuffer<float>& buffer)
    {
        if (bypassed.load (std::memory_order_acquire))  // Atomic read
            return;

        float currentGain = gainSmoothed.getNextValue();  // Thread-safe
        // ... apply gain
    }

private:
    juce::SmoothedValue<float> gainSmoothed;  // Thread-safe by design
    std::atomic<bool> bypassed { false };
};
```

**Never use regular locks in audio thread**

```cpp
// BAD - Mutex in audio thread
class BadProcessor
{
    void processBlock (...)
    {
        std::lock_guard<std::mutex> lock (mutex);  // FORBIDDEN!
        // ... processing
    }

    void setParameter (float value)
    {
        std::lock_guard<std::mutex> lock (mutex);
        paramValue = value;
    }

    std::mutex mutex;
};

// GOOD - Lock-free alternative
class GoodProcessor
{
    void processBlock (...)
    {
        float value = paramValue.load (std::memory_order_relaxed);  // Lock-free!
        // ... processing
    }

    void setParameter (float value)
    {
        paramValue.store (value, std::memory_order_relaxed);  // Lock-free!
    }

    std::atomic<float> paramValue { 0.0f };
};
```

---

## 4. Modern C++ Best Practices

### 4.1 Smart Pointer Usage

**Use `std::unique_ptr` for exclusive ownership**
```cpp
// Good - unique ownership
class MyProcessor
{
public:
    MyProcessor()
        : parameterListener (std::make_unique<ParameterListener> (apvts, dsp))
    {
    }

private:
    std::unique_ptr<ParameterListener> parameterListener;
};

// Bad - raw pointer with manual memory management
class MyProcessor
{
public:
    MyProcessor()
    {
        parameterListener = new ParameterListener (...);  // Manual allocation
    }

    ~MyProcessor()
    {
        delete parameterListener;  // Manual cleanup - error-prone
    }

private:
    ParameterListener* parameterListener;  // Raw owning pointer
};
```

**Use `std::shared_ptr` only when shared ownership is needed**
```cpp
// Good - shared ownership is actually needed
class PluginManager
{
    std::shared_ptr<AudioEngine> engine;  // Shared between manager and workers
};

// Bad - unnecessary shared ownership
class MyComponent
{
    std::shared_ptr<Label> label;  // Overkill - use std::unique_ptr or direct member
};

// Better
class MyComponent
{
    juce::Label label;  // Direct member (preferred for components)
    // or
    std::unique_ptr<juce::Label> label;  // Unique ownership if needed
};
```

**Non-owning pointers** - Use raw pointers (but document ownership)
```cpp
// Good - non-owning pointer (documented)
class MyEditor
{
public:
    void setProcessor (MyProcessor* processor)  // Non-owning, lifetime managed elsewhere
    {
        audioProcessor = processor;
    }

private:
    MyProcessor* audioProcessor = nullptr;  // Non-owning (editor doesn't control lifetime)
};
```

### 4.2 RAII Patterns

**Resource Acquisition Is Initialization - Use RAII for all resources**

```cpp
// Good - RAII with smart pointers
void processAudio()
{
    auto buffer = std::make_unique<juce::AudioBuffer<float>> (2, 512);
    // buffer automatically cleaned up when function exits (exception-safe)
}

// Good - RAII with JUCE utilities
void renderComponent()
{
    juce::ScopedNoDenormals noDenormals;  // CPU state restored automatically

    // ... render code

}  // CPU state restored here

// Good - Custom RAII wrapper
class ScopedAudioLock
{
public:
    explicit ScopedAudioLock (juce::AudioProcessor& p) : processor (p)
    {
        processor.suspendProcessing (true);
    }

    ~ScopedAudioLock()
    {
        processor.suspendProcessing (false);
    }

private:
    juce::AudioProcessor& processor;
};

// Bad - manual resource management
void processAudio()
{
    float* buffer = new float[512];

    // ... processing

    delete[] buffer;  // Easy to forget, no exception safety
}
```

### 4.3 Const Correctness

**Use `const` wherever possible**

```cpp
// Good - const correctness
class MyDSP
{
public:
    float getGain() const { return currentGain; }  // Const method

    void process (const juce::AudioBuffer<float>& input,  // Const input
                  juce::AudioBuffer<float>& output);       // Mutable output

    void setParameter (const juce::String& name, float value);  // Const reference

private:
    float currentGain = 1.0f;
    const int maxChannels = 8;  // Const member
};

// Bad - missing const
class MyDSP
{
public:
    float getGain() { return currentGain; }  // Should be const

    void process (juce::AudioBuffer<float>& input,  // Should be const
                  juce::AudioBuffer<float>& output);
};
```

**Const references for parameters**
```cpp
// Good - const reference for non-primitive types
void setTitle (const juce::String& title);
void process (const std::vector<float>& input);

// Bad - copy by value (expensive for large types)
void setTitle (juce::String title);  // Unnecessary copy

// Good - pass by value for primitives and small types
void setGain (float gain);           // Primitives by value
void setEnabled (bool enabled);      // Small types by value
```

**Mark local parameters `const` when they are not modified**

If a function parameter is never written to inside the function body, declare it `const`. This documents intent, prevents accidental mutation, and enables compiler optimisations.

```cpp
// Good - const parameters communicate that values are not modified
void applyGain (const float gain, juce::AudioBuffer<float>& buffer)
{
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
        buffer.applyGain (ch, 0, buffer.getNumSamples(), gain);
}

int samplesToSeconds (const int numSamples, const double sampleRate)
{
    return static_cast<int> (static_cast<double> (numSamples) / sampleRate);
}

// Bad - parameter could be const but isn't
void applyGain (float gain, juce::AudioBuffer<float>& buffer)  // gain is never modified
{
    buffer.applyGain (gain);
}

// Good - local variables that don't change should also be const
void process (juce::AudioBuffer<float>& buffer)
{
    const int numSamples = buffer.getNumSamples();
    const int numChannels = buffer.getNumChannels();

    for (int ch = 0; ch < numChannels; ++ch)
        for (int i = 0; i < numSamples; ++i)
            buffer.setSample (ch, i, buffer.getSample (ch, i) * 0.5f);
}
```

**Prefer `constexpr` for compile-time constants**

If a variable's value is known at compile time and never changes, prefer `constexpr` over `const`. `constexpr` is evaluated at compile time, produces no runtime overhead, and clearly signals that the value is a true constant.

```cpp
// Good - constexpr for compile-time constants
static constexpr int fifoSize = 8192;
static constexpr float defaultGainDB = 0.0f;
static constexpr double twoPi = 2.0 * juce::MathConstants<double>::pi;

namespace ParameterDefaults
{
    inline constexpr float gainDefault = 0.0f;
    inline constexpr float dryWetDefault = 100.0f;
    inline constexpr int maxDelayMs = 2000;
}

// Good - constexpr local variable
void calculateCoefficients (double sampleRate)
{
    constexpr double referenceFrequency = 440.0;
    const double normalised = referenceFrequency / sampleRate;  // runtime value: const
    // ...
}

// Bad - const where constexpr is possible
static const int fifoSize = 8192;    // Value is known at compile time; use constexpr
const float pi = 3.14159265f;        // Use constexpr (or juce::MathConstants)

// Bad - #define for numeric constants
#define FIFO_SIZE 8192               // No type safety, no scope; use constexpr
```

### 4.4 nullptr vs NULL

**Always use `nullptr`, never `NULL` or `0`**

```cpp
// Good
MyClass* ptr = nullptr;
if (ptr != nullptr)
    ptr->doSomething();

// Bad
MyClass* ptr = NULL;   // C-style
MyClass* ptr = 0;      // Integer literal
if (ptr)               // Implicit bool conversion (acceptable but less clear)
```

### 4.5 auto Usage Guidelines

**Use `auto` for:**
- Iterator types
- Complex template types
- Lambda expressions
- When type is obvious from initializer

```cpp
// Good - auto improves readability
auto it = myMap.begin();  // Iterator type
auto lambda = [] (int x) { return x * 2; };
auto buffer = std::make_unique<juce::AudioBuffer<float>> (2, 512);
auto spec = juce::dsp::ProcessSpec { 44100.0, 512, 2 };

// Good - explicit type when clarity is important
juce::AudioBuffer<float> buffer (2, 512);  // Clear intent
int sampleCount = calculateSamples();      // Obvious type
```

**Avoid `auto` when:**
- Type is not obvious from context
- Proxy objects are involved
- Clarity is reduced

```cpp
// Bad - type unclear
auto result = someFunction();  // What type is this?

// Better - explicit type
float result = someFunction();

// Bad - proxy object issue
auto value = someVector[0];  // May be proxy, not actual value

// Better
float value = someVector[0];  // Explicit conversion
```

**Use `auto*` and `auto&` for pointers and references**
```cpp
// Good - explicit pointer/reference
auto* ptr = getPointer();
auto& ref = getReference();
const auto& constRef = getConstReference();

// Acceptable but less clear
auto ptr = getPointer();  // Is it a pointer or value?
```

### 4.6 Class Template Argument Deduction (CTAD)

**Let the compiler deduce template arguments when the type is unambiguous.**

C++17 CTAD eliminates redundant template parameters when they can be inferred from the constructor arguments. Prefer deduced forms to reduce noise — explicit template arguments are only needed when deduction would be ambiguous or produce a surprising type.

```cpp
// Good - compiler deduces <float> from the float arguments
juce::Point startPoint (static_cast<float> (x), static_cast<float> (y));
juce::Rectangle tickBounds (x, y, w, h);  // all floats → Rectangle<float>

juce::ColourGradient gradient (colorA, x1, y1, colorB, x2, y2, false);

std::pair result (42, 3.14f);      // deduced as pair<int, float>
std::array values { 1.0f, 2.0f };  // deduced as array<float, 2>

// Good - explicit template arg required: deduction would be ambiguous
juce::Rectangle<int> bounds (x, y, w, h);   // x/y/w/h are int; would also deduce — explicit for clarity
std::vector<float> samples;                  // no constructor args to deduce from

// Bad - redundant explicit template argument
juce::Point<float> startPoint (static_cast<float> (x), static_cast<float> (y));  // <float> is obvious
juce::Rectangle<float> tickBounds (x, y, w, h);  // redundant when all args are float
```

**When to be explicit:**
- Deduction would pick the wrong type (`int` vs `float`, signed vs unsigned)
- Empty or no-arg constructors (nothing to deduce from)
- Readability genuinely benefits from seeing the type (e.g. distinguishing `Rectangle<int>` from `Rectangle<float>` in a bounds-heavy context)

### 4.7 Type Casting

**Always use C++ named casts instead of C-style casts**

C-style casts (`(int)x`, `(float)x`) are unsafe because they silently attempt multiple kinds of conversions in an unspecified order. C++ named casts are explicit about intent and safer.

| Cast | Use when |
|------|----------|
| `static_cast<T>` | Safe, well-defined conversions (numeric types, upcasts, explicit conversions) |
| `reinterpret_cast<T>` | Reinterpreting raw bit patterns (use sparingly, document why) |
| `const_cast<T>` | Removing `const` qualifier (use only when unavoidable, e.g. legacy API) |
| `dynamic_cast<T>` | Downcasting with runtime type check (prefer JUCE-style or avoid deep hierarchies) |

```cpp
// Good - C++ named casts
int numSamples = static_cast<int> (buffer.getNumSamples());
float phase = static_cast<float> (sampleIndex) / static_cast<float> (bufferSize);
auto* derived = static_cast<MyDerivedComponent*> (parentComponent);

juce::uint32 blockSize = static_cast<juce::uint32> (samplesPerBlock);

// Good - reinterpret_cast for raw memory (rare, document intent)
// Reinterpret PCM byte array as interleaved float samples
const float* samples = reinterpret_cast<const float*> (rawBytes);

// Bad - C-style casts
int numSamples = (int) buffer.getNumSamples();  // Silent, ambiguous conversion
float phase = (float) sampleIndex / (float) bufferSize;
auto* derived = (MyDerivedComponent*) parentComponent;  // No safety check
```

**Prefer `static_cast` for numeric conversions (addresses the "implicit conversion" warning):**

```cpp
// Good - explicit conversion, no compiler warning
void prepareToPlay (double sampleRate, int samplesPerBlock) override
{
    auto delayBufferSize = static_cast<int> (sampleRate * 2.0);
    delayBuffer.setSize (2, delayBufferSize);

    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = static_cast<juce::uint32> (samplesPerBlock);
    spec.numChannels = static_cast<juce::uint32> (getTotalNumOutputChannels());
}

// Bad - implicit conversion or C-style cast
spec.maximumBlockSize = (juce::uint32) samplesPerBlock;  // C-style
spec.maximumBlockSize = samplesPerBlock;                  // Implicit, may warn
```

### 4.8 Lambda Best Practices

**Prefer lambdas for short callbacks and algorithms**

```cpp
// Good - lambda for simple callback
button.onClick = [this]
{
    audioProcessor.setParameter (paramID, slider.getValue());
};

// Good - lambda with algorithm
std::transform (input.begin(), input.end(), output.begin(),
                [] (float sample) { return sample * 0.5f; });

// Good - lambda with explicit capture
timer.startTimer (100, [processor = &audioProcessor, this]
{
    updateDisplay (processor->getCurrentLevel());
});
```

**Be explicit about capture**
```cpp
// Good - explicit capture by reference
auto lambda = [&buffer, sampleRate] ()
{
    processBuffer (buffer, sampleRate);
};

// Good - explicit capture by value
auto lambda = [gain, mix] ()
{
    return calculateOutput (gain, mix);
};

// Dangerous - capture all by reference
auto lambda = [&] ()  // May capture dangling references
{
    // If this lambda outlives the scope, references are invalid
};

// Bad - capture all by value (unclear dependencies)
auto lambda = [=] ()
{
    // What are we capturing? Unclear from reading code
};
```

**Mark lambdas as `mutable` when needed**
```cpp
// Good - mutable lambda
auto counter = [count = 0] () mutable
{
    return count++;  // Modifies captured variable
};

// Note: Avoid mutable lambdas in audio thread (not realtime-safe)
```

---

## 5. SOLID & GRASP Design Principles

This section outlines the core object-oriented design principles applied throughout the gFractor codebase. These principles promote maintainability, testability, and extensibility.

### 5.1 SOLID Principles

#### 5.1.1 Single Responsibility Principle (SRP)

**Definition:** A class should have only one reason to change.

**Application in gFractor:**
```cpp
// Good - each class has one responsibility
class SinkRegistry {
    // Only manages audio data sink registration and dispatch
};

class PerformanceMonitor {
    // Only tracks and reports performance metrics
};

class gFractorAudioProcessor {
    // Orchestrates audio processing, delegates to specialized classes
    // Does NOT directly manage sinks or compute metrics
};
```

**When to apply:**
- When a class grows beyond ~200 lines
- When testing requires mocking multiple concerns
- When unrelated changes frequently affect the same class

#### 5.1.2 Open/Closed Principle (OCP)

**Definition:** Classes should be open for extension but closed for modification.

**Application in gFractor:**
```cpp
// Good - use interfaces for extensibility
class IAudioDataSink {
public:
    virtual ~IAudioDataSink() = default;
    virtual void pushStereoData(const juce::AudioBuffer<float>& buffer) = 0;
    virtual void setSampleRate(double sr) = 0;
};

// Add new sinks without modifying existing code
class SpectrumAnalyzer : public IAudioDataSink { /* ... */ };
class WaveformDisplay : public IAudioDataSink { /* ... */ };
```

**When to apply:**
- When adding new channel modes (M/S, L/R, T/N)
- When introducing new filter types
- When creating new visualizer components

#### 5.1.3 Liskov Substitution Principle (LSP)

**Definition:** Objects of a superclass should be replaceable with objects of a subclass without breaking the application.

**Application in gFractor:**
```cpp
// Good - interfaces define complete contracts
class IAudioDataSink {
public:
    virtual ~IAudioDataSink() = default;
    virtual void pushStereoData(const juce::AudioBuffer<float>& buffer) = 0;
    virtual void setSampleRate(double sr) = 0;
    // All methods must be implemented - no partial interfaces
};

// Implementations must satisfy full contract
class SpectrumAnalyzer : public IAudioDataSink {
    void pushStereoData(const juce::AudioBuffer<float>& buffer) override { /* ... */ }
    void setSampleRate(double sr) override { /* ... */ }
};
```

**When to apply:**
- Always use interfaces with complete contracts
- Avoid "partial" interface implementations
- Ensure subclasses honor preconditions/postconditions of base

#### 5.1.4 Interface Segregation Principle (ISP)

**Definition:** Prefer small, focused interfaces over large, general-purpose ones.

**Application in gFractor:**
```cpp
// Good - separate interfaces for different concerns
class IAudioDataSink {
    virtual void pushStereoData(const juce::AudioBuffer<float>& buffer) = 0;
    virtual void setSampleRate(double sr) = 0;
};

class IGhostDataSink {
    virtual void pushGhostData(const juce::AudioBuffer<float>& buffer) = 0;
};

class IPeakLevelSource {
    virtual float getPeakPrimaryDb() const = 0;
    virtual float getPeakSecondaryDb() const = 0;
};

// Bad - bloated interface
class IAllInOneSink {
    virtual void pushAudio() = 0;
    virtual void pushGhost() = 0;
    virtual void setSampleRate() = 0;
    virtual float getPeak() = 0;
    // Too many responsibilities
};
```

**When to apply:**
- When interfaces have more than 5-7 methods
- When implementers only need a subset of methods
- When testing requires mocking only specific behavior

#### 5.1.5 Dependency Inversion Principle (DIP)

**Definition:** Depend on abstractions, not concretions. High-level modules should not depend on low-level modules.

**Application in gFractor:**
```cpp
// Good - depend on interface, inject implementation
class gFractorAudioProcessor {
private:
    gFractorDSP& dspProcessor;  // Concrete, but owned by processor
    SinkRegistry sinkRegistry;   // Concrete, focused class
    
    // For testability, consider interface for DSP:
    // IDSPProcessor& dspProcessor;  // Interface
};

// Concrete classes are fine when they are stable and self-contained
// gFractorDSP - stable, well-tested core processing
// SinkRegistry - simple coordination, no complex dependencies
```

**When to apply:**
- When a class is difficult to test due to concrete dependencies
- When dependencies might change (e.g., different DSP strategies)
- When implementing strategy patterns

### 5.2 GRASP Principles

#### 5.2.1 Information Expert

**Definition:** Assign responsibility to the class that has the information needed to fulfill it.

**Application in gFractor:**
```cpp
// Good - PerformanceMonitor owns its metrics
class PerformanceMonitor {
    struct Metrics metrics;
public:
    const Metrics& getMetrics() const { return metrics; }
    void recordBlock(double elapsedMs, double sampleRate, int blockSize);
};

// PluginProcessor delegates to PerformanceMonitor
void gFractorAudioProcessor::processBlock(...) {
    perfMonitor.recordBlock(elapsedMs, getSampleRate(), buffer.getNumSamples());
}
```

#### 5.2.2 Creator

**Definition:** Assign responsibility for creating objects to a class that contains, uses, or has information about the class to be created.

**Application in gFractor:**
```cpp
// Good - PluginProcessor creates and owns DSP processor
class gFractorAudioProcessor {
private:
    gFractorDSP dspProcessor;  // Created and owned by processor
    std::unique_ptr<ParameterListener> parameterListener;
};

// Good - Editor creates visualizers it owns
class gFractorAudioProcessorEditor {
    std::unique_ptr<SpectrumAnalyzer> spectrumAnalyzer;
};
```

#### 5.2.3 Controller

**Definition:** Assign responsibility for handling a system operation to a class representing the system or a use case.

**Application in gFractor:**
```cpp
// Good - gFractorAudioProcessor coordinates processing
void gFractorAudioProcessor::processBlock(AudioBuffer<float>& buffer, MidiBuffer& midi) {
    // 1. Push data to sinks
    sinkRegistry.pushAudioData(...);
    
    // 2. Process through DSP
    dspProcessor.process(buffer);
    
    // 3. Record performance
    perfMonitor.recordBlock(...);
}

// Controller doesn't do the work - delegates to specialists
```

#### 5.2.4 Low Coupling

**Definition:** Assign responsibilities to minimize dependencies between classes.

**Application in gFractor:**
```cpp
// Good - low coupling via interfaces
class IAudioDataSink { /* minimal interface */ };
class SpectrumAnalyzer : public IAudioDataSink { /* ... */ };

// gFractorAudioProcessor depends on interface, not concrete visualizer
void registerAudioDataSink(IAudioDataSink* sink);

// Good - event-based communication reduces direct coupling
class ParameterListener {
    // Listens to APVTS changes, updates DSP
    // Decouples parameter system from DSP implementation
};
```

#### 5.2.5 High Cohesion

**Definition:** Assign responsibilities so that classes remain focused and well-understood.

**Application in gFractor:**
```cpp
// Good - high cohesion in SinkRegistry
class SinkRegistry {
    // Focused on: sink registration, data dispatch
    void registerAudioDataSink(IAudioDataSink* sink);
    void unregisterAudioDataSink(IAudioDataSink* sink);
    void pushAudioData(...);
    void pushGhostData(...);
    // All related to sink management
};

// Bad - low cohesion
class AudioProcessorHelper {
    // Does audio processing
    // Manages sinks
    // Computes performance metrics
    // Handles parameter serialization
    // Too many unrelated responsibilities
};
```

### 5.3 Summary Checklist

**Before writing a new class, verify:**

- [ ] Does this class have a single, clear responsibility?
- [ ] Can it be extended without modifying existing code?
- [ ] Do subclasses fully implement parent interfaces?
- [ ] Are interfaces small and focused?
- [ ] Does this class depend on abstractions where beneficial?
- [ ] Is the information needed to fulfill responsibilities held by this class?
- [ ] Does this class have low coupling and high cohesion?

**Refactoring triggers:**
- Class exceeds ~200 lines
- Class has more than 5 unrelated methods
- Changes to one area require changes in unrelated areas
- Testing requires extensive mocking of unrelated concerns

---

## 6. Project-Specific Conventions

### 6.1 File Organization

### 6.2 Folder Structure Patterns

This section defines patterns for organizing code to maintain logical grouping and discoverability.

#### 6.2.1 Grouping Principles

| Principle | Description | Example |
|-----------|-------------|---------|
| **Cohesion** | Files that change together stay together | All FFT-related code in DSP/ |
| **Abstraction** | Interfaces near implementations | IAudioDataSink.h next to AudioVisualizer.cpp |
| **Dependency** | Lower-level code in subdirectories | DSP/ contains core processing, UI/ depends on it |
| **Ownership** | Components owned by same subsystem | Visualizers in UI/Visualizers/, panels in UI/Panels/ |

#### 6.2.2 Directory Organization

```
Source/
├── DSP/                        # Core audio processing (no UI dependencies)
│   ├── Core/                   # Main processing
│   │   ├── gFractorDSP.h/.cpp # Main DSP processor
│   │   └── DSPConstants.h     # Processing constants
│   │
│   ├── Interfaces/             # Abstractions (I prefix)
│   │   ├── IAudioDataSink.h
│   │   ├── IGhostDataSink.h
│   │   ├── IDSPProcessor.h
│   │   ├── IFilterBank.h
│   │   ├── IGainProcessor.h
│   │   ├── ITransientDetector.h
│   │   └── IPeakLevelSource.h
│   │
│   ├── Analysis/               # FFT & audio analysis
│   │   ├── FFTProcessor.h/.cpp
│   │   └── AudioRingBuffer.h/.cpp
│   │
│   └── Monitoring/             # Performance & data flow
│       ├── PerformanceMonitor.h/.cpp
│       └── SinkRegistry.h/.cpp
│
├── State/                      # Parameter & preset management
│   ├── ParameterIDs.h          # Parameter identifier constants
│   ├── ParameterLayout.h/.cpp  # APVTS layout definition
│   ├── ParameterDefaults.h     # Default values
│   ├── ParameterListener.h     # APVTS → DSP sync
│   └── PluginState.h/.cpp     # Serialization
│
├── UI/                         # User interface components
│   ├── Controls/               # Reusable UI controls
│   │   ├── PillButton.h
│   │   ├── DropdownPill.h
│   │   └── HintBar.h/.cpp
│   ├── Panels/                 # Major UI panels
│   │   ├── PreferencePanel.h/.cpp
│   │   └── StereoMeteringPanel.h/.cpp
│   ├── Visualizers/            # Audio visualization components
│   │   ├── SpectrumAnalyzer.h/.cpp
│   │   ├── GhostSpectrum.h/.cpp
│   │   └── PeakHold.h/.cpp
│   └── Theme/                  # Visual styling
│       ├── ColorPalette.h
│       ├── Typography.h
│       └── Spacing.h
│
├── Utility/                    # Cross-cutting utilities
│   ├── ChannelMode.h           # Enum definitions
│   ├── DisplayRange.h          # Value objects
│   └── AnalyzerSettings.h
│
├── PluginProcessor.h/.cpp      # Main audio processor
└── PluginEditor.h/.cpp        # Main editor
```

#### 6.2.3 File Naming Conventions

```cpp
// Implementation + Header: same name, different extension
SinkRegistry.h      → SinkRegistry.cpp
PerformanceMonitor.h→ PerformanceMonitor.cpp

// Interfaces: I prefix, header only (no .cpp)
IAudioDataSink.h    // No IAudioDataSink.cpp

// Simple value objects: header only
ChannelMode.h       // No ChannelMode.cpp

// Groups of related constants: header only
ParameterIDs.h      // No ParameterIDs.cpp
ColorPalette.h      // No ColorPalette.cpp
```

#### 6.2.4 Interface Placement

**Rule:** Place interfaces in the same directory as their primary implementer or consumer.

```cpp
// Good - interface near main implementation
Source/DSP/
├── IAudioDataSink.h           # Interface definition
├── SpectrumAnalyzer.h/.cpp   # Primary implementer
└── WaveformDisplay.h/.cpp    # Also implements IAudioDataSink

// Acceptable - interface in owning subsystem
Source/UI/
├── ISpectrumControls.h        # Used by multiple visualizers
└── Visualizers/
    ├── SpectrumAnalyzer.h/.cpp
    └── WaveformDisplay.h/.cpp
```

#### 6.2.5 When to Create a New Directory

Create a new subdirectory when:
1. A group of 3+ files share a distinct responsibility
2. The directory would contain both interfaces and implementations
3. Other directories would need to depend on this group

```cpp
// Good - distinct responsibility, multiple files
Source/DSP/
├── SinkRegistry.h/.cpp
├── PerformanceMonitor.h/.cpp
// New directory for coherent group
Source/Processing/
├── ChannelProcessor.h/.cpp
├── Crossover.h/.cpp
└── Limiter.h/.cpp

// Bad - single file or unclear boundary
Source/Processing/        // Only one file - don't create directory
    └── OneClass.h
```

#### 6.2.6 Dependency Direction

```
┌─────────────────────────────────────────────────────────┐
│                     PluginEditor                        │
│                  (depends on UI/)                      │
└─────────────────────┬───────────────────────────────────┘
                      │
        ┌─────────────┴─────────────┐
        ▼                           ▼
┌───────────────┐         ┌─────────────────┐
│     UI/       │         │  PluginProcessor│
│  Visualizers  │         │   (depends on)  │
│   Controls    │         │    DSP/, State/ │
│   Panels      │         └────────┬────────┘
└───────┬───────┘                  │
        │                 ┌────────┴────────┐
        │                 ▼                 ▼
        │          ┌───────────┐     ┌──────────┐
        └─────────▶│   DSP/   │     │  State/  │
                   │ (core)   │     │(params)   │
                   └───────────┘     └───────────┘

Rule: Dependencies flow inward. Core (DSP, State) has no UI dependencies.
```

#### 6.2.7 Summary Checklist

**Before creating a new file, verify:**

- [ ] Does this file belong in an existing directory?
- [ ] Is there a clear owner (directory) for this functionality?
- [ ] Does the file group with others that change for the same reasons?
- [ ] Are dependency directions correct (no UI → DSP dependencies)?
- [ ] If creating a new directory, will it contain 3+ related files?

**Directory structure review:**
- [ ] Core DSP has no UI #includes
- [ ] State/ has no UI #includes
- [ ] Interfaces use I prefix, header-only
- [ ] Implementation files paired with headers

---
│   ├── ChannelMode.h
│   ├── DisplayRange.h
│   ├── SpectrumAnalyzerDefaults.h
│   └── AnalyzerSettings.h
├── State/                  # Parameters & state management
│   ├── ParameterIDs.h
│   ├── ParameterDefaults.h
│   ├── ParameterLayout.h
│   ├── ParameterListener.h
│   └── PluginState.h/.cpp
├── UI/                     # User interface
│   ├── ISpectrumControls.h
│   ├── ISpectrumDisplaySettings.h
│   ├── Visualizers/
│   │   ├── SpectrumAnalyzer.h/.cpp
│   │   ├── AudioVisualizerBase.h/.cpp
│   │   ├── SonogramView.h/.cpp
│   │   ├── GhostSpectrum.h/.cpp
│   │   ├── PeakHold.h/.cpp
│   │   └── SpectrumTooltip.h/.cpp
│   ├── Panels/
│   │   ├── StereoMeteringPanel.h/.cpp
│   │   ├── PreferencePanel.h/.cpp
│   │   └── HelpPanel.h/.cpp
│   ├── Controls/
│   │   ├── HeaderBar.h/.cpp
│   │   ├── FooterBar.h/.cpp
│   │   ├── PillButton.h
│   │   └── PerformanceDisplay.h
│   ├── LookAndFeel/
│   │   └── gFractorLookAndFeel.h
│   └── Theme/
│       ├── ColorPalette.h
│       └── Spacing.h
├── PluginProcessor.h/.cpp  # Main audio processor
└── PluginEditor.h/.cpp     # Main editor
```

**File naming:**
- Header files: `.h` extension
- Source files: `.cpp` extension
- Match class name to filename: `WaveformVisualizer` -> `WaveformVisualizer.h`

### 6.3 Color Definitions Belong in `ColorPalette.h`

**Never hardcode color literals in components — add them to `Source/UI/Theme/ColorPalette.h`.**

All `juce::Colour` values used in the UI must be defined as `inline constexpr juce::uint32` constants in the `ColorPalette` namespace. This keeps the visual theme in one place, makes global color changes trivial, and prevents magic hex literals from scattering across the codebase.

```cpp
// Good - reference a named constant from ColorPalette
#include "UI/Theme/ColorPalette.h"

void MyComponent::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (ColorPalette::background));
    g.setColour (juce::Colour (ColorPalette::primaryGreen));
    g.drawText (label, bounds, juce::Justification::centred);
}

// Bad - magic hex literals inline in a component
void MyComponent::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (0xff0D0F0D));   // What color is this?
    g.setColour (juce::Colour (0xff3DCC6E)); // Duplicated, out-of-sync risk
}
```

**Adding a new color:**

1. Choose the appropriate semantic group in `ColorPalette.h` (backgrounds, text, accent, etc.)
2. Add an `inline constexpr juce::uint32` with a descriptive name
3. Reference it by name everywhere it is needed

```cpp
// ColorPalette.h — add to the relevant group
namespace ColorPalette
{
    // Accent colors
    inline constexpr juce::uint32 primaryGreen   = 0xff3DCC6E;
    inline constexpr juce::uint32 sideAmber  = 0xffC8A820;
    inline constexpr juce::uint32 blueAccent = 0xff1E6ECC;

    // New color added to the correct group:
    inline constexpr juce::uint32 warningRed = 0xffCC3D3D;
}
```

**Note:** Alpha-encoded colors use the `0xAARRGGBB` format (e.g. `0xb3ffb6c1` = light pink at 70% alpha). Document non-obvious alpha values with a short comment as shown in `ColorPalette.h`.

### 6.4 Header Guards vs `#pragma once`

**Use `#pragma once` (simpler, faster, no naming conflicts)**

```cpp
// Good - pragma once (project standard)
#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

class MyClass
{
    // ...
};

// Bad - header guards (verbose, error-prone)
#ifndef MYCLASS_H_INCLUDED
#define MYCLASS_H_INCLUDED

class MyClass { };

#endif  // MYCLASS_H_INCLUDED
```

**Note:** `#pragma once` is supported by all modern compilers and is the JUCE convention.

### 6.5 Forward Declarations

**Use forward declarations to reduce compile times**

```cpp
// Good - forward declaration in header
#pragma once

// Forward declarations
class WaveformVisualizer;
namespace juce { class AudioBuffer; }

class MyProcessor
{
public:
    void setVisualizer (WaveformVisualizer* vis);

private:
    WaveformVisualizer* visualizer = nullptr;  // Pointer, no need for full definition
};

// Bad - unnecessary includes in header
#include "WaveformVisualizer.h"  // Only needed in .cpp if using pointer
```

**When you need full definition:**
- Inheritance
- Member variables (not pointers/references)
- Template instantiation
- Inline function implementation that uses the type

```cpp
// Need full definition
#include "BaseClass.h"
class MyClass : public BaseClass { };  // Inheritance

#include "Member.h"
class MyClass
{
    Member member;  // Member variable (not pointer)
};

// Forward declaration sufficient
class Member;
class MyClass
{
    Member* member;  // Pointer only
    Member& getMember();  // Return reference (impl in .cpp)
};
```

### 6.6 Include Order

**Standard include order (prevents hidden dependencies):**

1. Corresponding header (for .cpp files)
2. C++ standard library headers
3. Third-party library headers (including JUCE)
4. Project headers

```cpp
// Good - MyClass.cpp
#include "MyClass.h"               // 1. Corresponding header first

#include <vector>                  // 2. C++ standard library
#include <memory>
#include <algorithm>

#include <juce_audio_processors/juce_audio_processors.h>  // 3. JUCE
#include <juce_dsp/juce_dsp.h>

#include "DSP/Filter.h"            // 4. Project headers
#include "State/ParameterIDs.h"

// Bad - random order
#include "State/ParameterIDs.h"
#include "MyClass.h"
#include <juce_audio_processors/juce_audio_processors.h>
#include <vector>
```

**Use angle brackets for system/library includes, quotes for project includes:**

```cpp
// Good
#include <juce_audio_processors/juce_audio_processors.h>  // Library
#include "MyClass.h"                                       // Project

// Bad
#include "juce_audio_processors/juce_audio_processors.h"  // Should use <>
#include <MyClass.h>                                      // Should use ""
```

### 6.7 Namespace Usage

**Namespace guidelines:**
- Don't use `using namespace` in headers (pollutes namespace)
- Prefer explicit namespace qualification or limited using declarations in .cpp files
- Use inline namespaces for versioning if needed

```cpp
// HEADER FILE (.h)

// Good - no using directives in headers
#pragma once

#include <juce_core/juce_core.h>

class MyClass
{
public:
    void setName (const juce::String& name);  // Explicit qualification

private:
    juce::String name;
};

// Bad - using in header
#pragma once
#include <juce_core/juce_core.h>
using namespace juce;  // NEVER in headers! Pollutes all includers

class MyClass
{
    String name;  // Ambiguous namespace
};
```

```cpp
// SOURCE FILE (.cpp)

// Good - using declarations or directives in .cpp only
#include "MyClass.h"

using juce::String;  // Limited using declaration
// or
// using namespace juce;  // Acceptable in .cpp (but prefer limited using)

void MyClass::setName (const String& newName)
{
    name = newName;
}

// Also good - explicit qualification
void MyClass::setName (const juce::String& newName)
{
    name = newName;
}
```

**Project namespaces for utilities:**

```cpp
// Good - namespace for parameter constants
namespace ParameterIDs
{
    inline constexpr auto gain = "gain";
    inline constexpr auto dryWet = "dryWet";
}

namespace ParameterDefaults
{
    inline constexpr float gainDefault = 0.0f;
    inline constexpr float dryWetDefault = 100.0f;
}

// Usage
void MyClass::initialize()
{
    apvts.getParameter (ParameterIDs::gain)->setValue (ParameterDefaults::gainDefault);
}
```

---

## 7. Error Handling

### 7.1 When to Use Assertions vs Exceptions

**Use `jassert` for:**
- Debug-time checks (removed in release builds)
- Precondition validation
- Postcondition validation
- Invariant checks

```cpp
// Good - jassert for preconditions
void processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    jassert (isPrepared);  // Check prepareToPlay was called
    jassert (buffer.getNumSamples() <= maxBlockSize);

    if (!isPrepared)  // Also guard in release
        return;

    // ... processing
}
```

**Use exceptions sparingly (not in audio thread!):**
- File I/O errors (loading presets, etc.)
- Initialization failures
- Invalid user input in non-realtime code

```cpp
// Good - exception for file loading (non-audio thread)
juce::ValueTree loadPreset (const juce::File& file)
{
    if (!file.existsAsFile())
        throw std::runtime_error ("Preset file does not exist");

    auto xml = juce::parseXML (file);
    if (xml == nullptr)
        throw std::runtime_error ("Failed to parse preset XML");

    return juce::ValueTree::fromXml (*xml);
}

// Bad - exception in audio thread
void processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    if (!isPrepared)
        throw std::runtime_error ("Not prepared");  // NEVER in audio thread!
}
```

**Prefer returning error codes or std::optional for audio-thread-safe error handling:**

```cpp
// Good - optional for error indication
std::optional<float> getSample (int index) const
{
    if (index < 0 || index >= buffer.getNumSamples())
        return std::nullopt;  // Error indication

    return buffer.getSample (0, index);
}

// Good - bool return for success/failure
bool loadState (const void* data, int size)
{
    if (data == nullptr || size == 0)
        return false;

    // ... load state
    return true;
}
```

### 7.2 jassert Usage

**Use `jassert` liberally in debug builds:**

```cpp
// Good - asserting preconditions
void setGain (float gainDB)
{
    jassert (gainDB >= -60.0f && gainDB <= 12.0f);  // Valid range check

    gain = juce::jlimit (-60.0f, 12.0f, gainDB);  // Also clamp in release
}

// Good - asserting invariants
void addSample (float sample)
{
    jassert (writePos < bufferSize);  // Invariant check
    jassert (! std::isnan (sample));   // Validate sample

    if (writePos >= bufferSize)  // Also guard in release
        return;

    buffer[writePos++] = sample;
}

// Good - asserting postconditions
juce::AudioBuffer<float> createBuffer (int channels, int samples)
{
    jassert (channels > 0 && samples > 0);

    juce::AudioBuffer<float> buffer (channels, samples);

    jassert (buffer.getNumChannels() == channels);  // Postcondition
    jassert (buffer.getNumSamples() == samples);

    return buffer;
}
```

**Don't use `jassert` for expected runtime errors:**

```cpp
// Bad - asserting user input
void loadFile (const juce::File& file)
{
    jassert (file.existsAsFile());  // User may select invalid file!

    // Better - handle error gracefully
    if (!file.existsAsFile())
    {
        juce::AlertWindow::showMessageBox (juce::AlertWindow::WarningIcon,
                                           "Error", "File not found");
        return;
    }
}
```

### 7.3 Error Logging Patterns

**Use JUCE's logging system:**

```cpp
// Good - logging in non-realtime code
void loadPreset (const juce::File& file)
{
    if (!file.existsAsFile())
    {
        juce::Logger::writeToLog ("Preset file not found: " + file.getFullPathName());
        return;
    }

    // ... load preset

    juce::Logger::writeToLog ("Loaded preset: " + file.getFileName());
}

// Bad - logging in audio thread
void processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::Logger::writeToLog ("Processing block");  // NEVER in processBlock!
}
```

**Use DBG macro for debug output (removed in release):**

```cpp
// Good - debug output (removed in release builds)
void setParameter (float value)
{
    DBG ("Parameter changed to: " + juce::String (value));
    // ... set parameter
}

// Note: DBG is fine even in frequently called code since it's removed in release
```

---

## 8. Documentation Standards

### 8.1 Function/Class Documentation Format

**Use Doxygen-style comments for public APIs:**

```cpp
/**
 * WaveformVisualizer Component
 *
 * A high-performance, thread-safe audio waveform visualizer for JUCE plugins.
 * Displays real-time input audio using a lock-free FIFO buffer to safely transfer
 * audio data from the audio thread to the UI thread.
 *
 * Features:
 * - Lock-free audio data transfer (realtime-safe)
 * - High-DPI/Retina display support
 * - Configurable update rate and display window size
 *
 * Usage Example:
 * @code
 * void processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
 * {
 *     waveformVisualizer.pushAudioData (buffer);
 * }
 * @endcode
 */
class WaveformVisualizer : public juce::Component
{
public:
    /**
     * Push audio data from the audio thread to the visualizer.
     * This method is realtime-safe and performs no allocations or locks.
     *
     * @param buffer The audio buffer to visualize (typically from processBlock)
     */
    void pushAudioData (const juce::AudioBuffer<float>& buffer);

    /**
     * Set the refresh rate in Hz
     * @param hz Refresh rate (higher = smoother but more CPU usage)
     */
    void setRefreshRate (int hz);
};
```

**Document important implementation details:**

```cpp
// Good - explaining non-obvious design decisions
void processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    // Apply gain with per-sample smoothing if needed
    // Smoothing prevents zipper noise when gain changes
    if (gainSmoothed.isSmoothing())
    {
        for (size_t sample = 0; sample < block.getNumSamples(); ++sample)
        {
            const float currentGain = gainSmoothed.getNextValue();
            // ... apply gain
        }
    }
    else
    {
        // Gain is stable, use optimized JUCE DSP processor
        gainProcessor.process (context);
    }
}
```

### 8.2 When to Write Comments

**DO comment:**
- Public API documentation
- Complex algorithms
- Non-obvious design decisions
- Workarounds for bugs or platform-specific issues
- Thread safety considerations
- Performance optimizations

```cpp
// Good - explaining complex algorithm
void calculateCoefficients()
{
    // Calculate filter coefficients using bilinear transform
    // See "Digital Signal Processing" by Oppenheim & Schafer, p. 450
    auto k = std::tan (juce::MathConstants<float>::pi * frequency / sampleRate);
    auto norm = 1.0f / (1.0f + k / q + k * k);

    b0 = k * k * norm;
    b1 = 2.0f * b0;
    b2 = b0;
    a1 = 2.0f * (k * k - 1.0f) * norm;
    a2 = (1.0f - k / q + k * k) * norm;
}

// Good - explaining thread safety
class MyDSP
{
    // Thread Safety: gainSmoothed is written from message thread via setGain()
    // and read from audio thread in process(). SmoothedValue handles
    // synchronization internally, making this safe.
    juce::SmoothedValue<float> gainSmoothed;
};
```

**DON'T comment:**
- Obvious code
- Redundant information
- What the code does (code should be self-documenting)

```cpp
// Bad - obvious comment
int count = 0;  // Initialize count to zero

// Bad - redundant comment
void setGain (float gain)
{
    // Set the gain
    this->gain = gain;
}

// Good - self-documenting code (no comment needed)
void setGain (float gain)
{
    this->gain = juce::jlimit (-60.0f, 12.0f, gain);
}
```

### 8.3 Documentation Generators

**This project uses Doxygen-style comments:**

- Use `/** ... */` for multi-line documentation
- Use `@param` for parameters
- Use `@return` for return values
- Use `@code ... @endcode` for code examples
- Use `@see` for cross-references

```cpp
/**
 * Calculate the output sample for the given input.
 *
 * This method applies the configured gain and dry/wet mix to the input sample.
 * It is realtime-safe and can be called from the audio thread.
 *
 * @param input The input sample value
 * @param channel The channel index (0 = left, 1 = right)
 * @return The processed output sample
 *
 * @see setGain(), setDryWet()
 */
float processSample (float input, int channel);
```

---

## 9. Testing Standards

### 9.1 Test File Naming

**Test files mirror source structure:**

```
Source/
├── DSP/
│   ├── gFractorDSP.h/.cpp
│   ├── FFTProcessor.h/.cpp
│   └── AudioRingBuffer.h/.cpp
└── PluginProcessor.cpp

Tests/
├── BasicTests.cpp        # Basic sanity tests
├── CoreTests.cpp         # AudioRingBuffer, ChannelDecoder, PeakHold, PluginState
├── DSPTests.cpp          # DSP processing tests
└── UIUtilityTests.cpp    # DisplayRange, FFTProcessor, defaults
```

**Test file naming convention:**
- `<Module>Tests.cpp` for module-specific tests
- `BasicTests.cpp` for framework validation
- Keep all tests in `Tests/` directory

### 9.2 Test Organization

**Use JUCE's UnitTest framework:**

```cpp
// Good - organized test class
class DSPTests : public juce::UnitTest
{
public:
    DSPTests() : juce::UnitTest ("DSP Tests", "Audio Processing") {}

    void runTest() override
    {
        // Group related tests
        testPrepareAndReset();
        testGainProcessing();
        testDryWetMixing();
        testBypassFunctionality();
        testParameterSmoothing();
    }

private:
    void testGainProcessing()
    {
        beginTest ("Gain Processing");

        // Test unity gain
        {
            gFractorDSP dsp;
            // ... test code
            expectWithinAbsoluteError (output, expectedOutput, 0.01f);
        }

        // Test +6 dB gain
        {
            gFractorDSP dsp;
            // ... test code
        }
    }

    // Helper methods at bottom
    void fillBufferWithValue (juce::AudioBuffer<float>& buffer, float value)
    {
        // ... helper implementation
    }
};

// Register the test
static DSPTests dspTests;
```

### 9.3 When to Write Tests

**Write tests for:**
- All DSP algorithms (gain, filters, effects)
- Parameter handling and smoothing
- State serialization/deserialization
- Edge cases and boundary conditions
- Bug fixes (regression tests)

```cpp
// Good - comprehensive DSP testing
void testGainProcessing()
{
    beginTest ("Gain Processing");

    // Test normal operation
    testUnityGain();
    testPositiveGain();
    testNegativeGain();

    // Test edge cases
    testZeroGain();
    testMaxGain();
    testMinGain();

    // Test with different signals
    testWithSilence();
    testWithFullScale();
    testWithVariousFrequencies();
}
```

**Don't write tests for:**
- Trivial getters/setters
- JUCE framework code (already tested)
- Pure UI layout code (hard to test, low value)

### 9.4 Test Coverage Expectations

**Coverage goals:**
- DSP code: 80-90% coverage (critical path)
- Parameter handling: 70-80% coverage
- State management: 70-80% coverage
- UI code: 30-50% coverage (focus on logic, not layout)

**Test categories:**
```cpp
// Unit tests - isolated component testing
class DSPTests : public juce::UnitTest
{
    // Test individual DSP components in isolation
};

// Integration tests - multiple components together
class ProcessorIntegrationTests : public juce::UnitTest
{
    // Test Processor + DSP + Parameters working together
};

// Regression tests - prevent bugs from reoccurring
class RegressionTests : public juce::UnitTest
{
    // Test for specific bugs that were fixed
    void testIssue42_ClickingOnParameterChange() { ... }
};
```

**Test assertions:**
```cpp
// Good - using appropriate assertions
void testGainProcessing()
{
    beginTest ("Gain Processing");

    expect (result == expected);                              // Exact equality
    expectEquals (result, expected);                          // Same as expect
    expectWithinAbsoluteError (result, expected, 0.01f);     // Floating point
    expectGreaterThan (result, threshold);                    // Comparison
    expectLessThan (result, threshold);
    expectNotEquals (result, badValue);
}

// Bad - using wrong assertion type
expectEquals (floatResult, 0.5f);  // Bad: exact float comparison
// Better
expectWithinAbsoluteError (floatResult, 0.5f, 0.001f);
```

---

## Summary Checklist

When writing code, ask yourself:

- [ ] Does it follow the naming conventions (camelCase for functions/variables, PascalCase for classes)?
- [ ] Is it realtime-safe if called from the audio thread (no allocations, locks, or I/O)?
- [ ] Are smart pointers used instead of raw owning pointers?
- [ ] Is `const` used wherever possible (methods, parameters, local variables)?
- [ ] Are compile-time constants declared `constexpr` rather than `const` or `#define`?
- [ ] Are JUCE types used appropriately (String, File, Component)?
- [ ] Is the code self-documenting, with comments only for complex/non-obvious sections?
- [ ] Does it use `nullptr` instead of `NULL`?
- [ ] Are C++ named casts (`static_cast`, `reinterpret_cast`) used instead of C-style casts?
- [ ] Are resources managed with RAII?
- [ ] Are all UI colors defined in `ColorPalette.h` (no hardcoded hex literals in components)?
- [ ] Is `#pragma once` used instead of header guards?
- [ ] Are includes ordered correctly (corresponding header, std, JUCE, project)?
- [ ] Are tests written for new DSP or parameter handling code?
- [ ] Does it compile without warnings (use `-Wall -Wextra -Wpedantic`)?

---

## Enforcement

**Automated tools:**
- `clang-format` - Code formatting (configuration: `.clang-format`)
- `clang-tidy` - Static analysis
- GitHub Actions - CI/CD with automatic checks

**Manual review:**
- All code changes require peer review
- Technical lead reviews architecture and realtime safety
- Focus code reviews on:
  - Thread safety
  - Memory management
  - API design
  - Performance implications

**Continuous improvement:**
- Update this document as patterns evolve
- Document exceptions and rationale
- Share learnings from code reviews

---

## References

- [JUCE Coding Standards](https://juce.com/learn/documentation)
- [C++ Core Guidelines](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines)
- [Real-Time Audio Programming 101](http://www.rossbencina.com/code/real-time-audio-programming-101-time-waits-for-nothing)
- [JUCE Forum Best Practices](https://forum.juce.com/)
- Will Pirkle's "Designing Audio Effect Plugins in C++"

---

**Document Version:** 1.0
**Maintained By:** Technical Lead
**Last Review:** 2026-02-16
