/*
  Unit tests for HintManager — crash safety, RAII handle lifecycle, priority ordering.

  These tests run on the message thread (guaranteed by ScopedJuceInitialiser_GUI in
  BasicTests.cpp's main()), satisfying HintManager's message-thread assertions.
*/

#include <juce_gui_basics/juce_gui_basics.h>
#include "UI/HintManager.h"

class HintManagerTests : public juce::UnitTest {
public:
    HintManagerTests() : UnitTest("HintManager Tests", "UI") {}

    void runTest() override {
        beginTest("Persistent hint shown when stack is empty");
        {
            HintManager mgr;
            HintManager::HintContent received;
            mgr.setCallback([&](const HintManager::HintContent& c) { received = c; });

            mgr.setPersistentHint("KEY", "fallback hint");

            expectEquals(received.title, juce::String("KEY"));
            expectEquals(received.hint,  juce::String("fallback hint"));
        }

        beginTest("RAII handle clears hint on destruction");
        {
            HintManager mgr;
            HintManager::HintContent received;
            mgr.setCallback([&](const HintManager::HintContent& c) { received = c; });

            mgr.setPersistentHint("KEY", "persistent");

            {
                auto h = mgr.setHint("CLICK", "transient");
                expectEquals(received.title, juce::String("CLICK"));
                expectEquals(received.hint,  juce::String("transient"));
            } // handle destroyed here

            // Must revert to persistent
            expectEquals(received.title, juce::String("KEY"));
            expectEquals(received.hint,  juce::String("persistent"));
        }

        beginTest("Move-assign to {} clears hint");
        {
            HintManager mgr;
            HintManager::HintContent received;
            mgr.setCallback([&](const HintManager::HintContent& c) { received = c; });

            mgr.setPersistentHint("KEY", "persistent");

            HintManager::HintHandle h = mgr.setHint("DRAG", "dragging");
            expectEquals(received.title, juce::String("DRAG"));

            h = {}; // move-assign empty handle => clears hint
            expectEquals(received.title, juce::String("KEY"));
        }

        beginTest("Priority ordering — higher priority wins");
        {
            HintManager mgr;
            HintManager::HintContent received;
            mgr.setCallback([&](const HintManager::HintContent& c) { received = c; });

            auto hLow    = mgr.setHint("LOW",    "contextual", HintManager::Contextual);
            auto hHigh   = mgr.setHint("HIGH",   "hover",      HintManager::Hover);
            auto hUrgent = mgr.setHint("URGENT", "urgent",     HintManager::Urgent);

            expectEquals(received.title, juce::String("URGENT"));

            hUrgent = {};
            expectEquals(received.title, juce::String("HIGH"));

            hHigh = {};
            expectEquals(received.title, juce::String("LOW"));

            hLow = {}; // all transient hints gone — no persistent set
            expectEquals(received.title, juce::String());
        }

        beginTest("Multiple handles — no crash on sequential destruction");
        {
            HintManager mgr;
            mgr.setCallback([](const HintManager::HintContent&) {});
            mgr.setPersistentHint("KEY", "persistent");

            auto h1 = mgr.setHint("CLICK", "one");
            auto h2 = mgr.setHint("DRAG",  "two");
            auto h3 = mgr.setHint("KEY",   "three");

            h2 = {};
            h1 = {};
            h3 = {};

            expect(true); // reaching here without crash is the goal
        }

        beginTest("Handle bool conversion reflects live state");
        {
            HintManager mgr;
            mgr.setCallback([](const HintManager::HintContent&) {});

            HintManager::HintHandle empty;
            expect(!empty);

            auto h = mgr.setHint("CLICK", "test");
            expect(static_cast<bool>(h));

            h = {};
            expect(!h);
        }

        beginTest("Move constructor transfers ownership");
        {
            HintManager mgr;
            HintManager::HintContent received;
            mgr.setCallback([&](const HintManager::HintContent& c) { received = c; });

            mgr.setPersistentHint("KEY", "persistent");

            HintManager::HintHandle h1 = mgr.setHint("CLICK", "test");
            HintManager::HintHandle h2 = std::move(h1);

            expect(!h1); // h1 released ownership
            expect(static_cast<bool>(h2));
            expectEquals(received.title, juce::String("CLICK"));

            h2 = {}; // destroy via the moved-into handle
            expectEquals(received.title, juce::String("KEY"));
        }

        beginTest("Callback fires on each push and pop");
        {
            HintManager mgr;
            int callCount = 0;
            mgr.setCallback([&](const HintManager::HintContent&) { ++callCount; });

            // setCallback itself fires once
            const int baseCount = callCount;

            auto h = mgr.setHint("CLICK", "a"); // +1 on push
            expect(callCount == baseCount + 1);

            h = {}; // +1 on pop
            expect(callCount == baseCount + 2);
        }

        beginTest("Same-priority hints — FIFO order (stable_sort preserves insertion order)");
        {
            HintManager mgr;
            HintManager::HintContent received;
            mgr.setCallback([&](const HintManager::HintContent& c) { received = c; });

            auto h1 = mgr.setHint("FIRST",  "first",  HintManager::Hover);
            auto h2 = mgr.setHint("SECOND", "second", HintManager::Hover);

            // stable_sort preserves insertion order for equal priorities: FIRST stays at front
            expectEquals(received.title, juce::String("FIRST"));

            h1 = {};
            expectEquals(received.title, juce::String("SECOND"));
        }

        beginTest("Safe destruction order — handles destroyed before manager");
        {
            // This is the correct usage pattern (handles are members of UI components
            // that outlive nothing beyond the editor, which holds the manager).
            // Handles must not crash when destroyed before the manager.
            {
                HintManager mgr;
                mgr.setCallback([](const HintManager::HintContent&) {});

                auto h1 = mgr.setHint("CLICK", "a");
                auto h2 = mgr.setHint("DRAG",  "b");
                h1 = {};
                h2 = {};
            } // mgr destroyed last — no crash expected

            expect(true);
        }

        beginTest("setPersistentHint updates callback immediately");
        {
            HintManager mgr;
            HintManager::HintContent received;

            // Wire callback after construction
            mgr.setCallback([&](const HintManager::HintContent& c) { received = c; });

            mgr.setPersistentHint("KEY", "v1");
            expectEquals(received.hint, juce::String("v1"));

            mgr.setPersistentHint("KEY", "v2");
            expectEquals(received.hint, juce::String("v2"));
        }

        beginTest("Transient hint masks persistent; pops back to persistent");
        {
            HintManager mgr;
            HintManager::HintContent received;
            mgr.setCallback([&](const HintManager::HintContent& c) { received = c; });

            mgr.setPersistentHint("KEY", "global shortcuts");

            {
                auto h = mgr.setHint("CLICK", "hover hint", HintManager::Hover);
                expectEquals(received.title, juce::String("CLICK"));
            }

            expectEquals(received.title, juce::String("KEY"));
            expectEquals(received.hint,  juce::String("global shortcuts"));
        }
    }
};

static HintManagerTests hintManagerTests;
