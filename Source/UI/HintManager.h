#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <algorithm>
#include <atomic>
#include <functional>
#include <memory>
#include <vector>

/**
 * HintManager
 *
 * Central coordinator for the HintBar's contextual text.
 * Each hint has a bold title (action type: "CLICK", "DRAG", "KEY", …)
 * and a normal-weight description.
 *
 * Priority levels (highest wins):
 *   Urgent     (30) — transient alerts, auto-expire via setHintAsync()
 *   Hover      (20) — mouse-hover contextual hints (most common)
 *   Contextual (10) — panel-level sustained hints
 *   Persistent  (0) — fallback set via setPersistentHint(), never auto-cleared
 *
 * Thread model:
 *   setHint() / setPersistentHint() — MESSAGE THREAD ONLY (all JUCE UI interactions)
 *   setHintAsync()                  — safe from any thread; posts to message thread
 */
class HintManager {
public:
    enum Priority { Persistent = 0, Contextual = 10, Hover = 20, Urgent = 30 };

    /** Two-part hint content: title rendered bold, hint rendered normal. */
    struct HintContent {
        juce::String title;  // e.g. "CLICK", "DRAG", "KEY"
        juce::String hint;   // description text
    };

    HintManager() : aliveFlag(std::make_shared<std::atomic<bool>>(true)) {}

    ~HintManager() { *aliveFlag = false; }

    //==========================================================================
    // RAII guard — hint is cleared when this is destroyed or move-assigned.
    // Store as a member variable; reassign to {} in mouseExit to auto-clear.
    class HintHandle {
    public:
        HintHandle() = default;

        ~HintHandle() { clear(); }

        HintHandle(HintHandle&& o) noexcept : manager(o.manager), id(o.id) {
            o.manager = nullptr;
        }

        HintHandle& operator=(HintHandle&& o) noexcept {
            if (this != &o) { clear(); manager = o.manager; id = o.id; o.manager = nullptr; }
            return *this;
        }

        void clear() { if (manager) { manager->clearHint(id); manager = nullptr; } }
        explicit operator bool() const { return manager != nullptr; }

        HintHandle(const HintHandle&) = delete;
        HintHandle& operator=(const HintHandle&) = delete;

    private:
        friend class HintManager;
        HintHandle(HintManager* m, const int i) : manager(m), id(i) {}
        HintManager* manager = nullptr;
        int id = -1;
    };

    //==========================================================================
    // Push a transient hint — returns a RAII HintHandle. Store it as a member;
    // the hint is removed when the handle is destroyed or move-assigned.
    // CALL ON MESSAGE THREAD ONLY.
    [[nodiscard]] HintHandle setHint(const juce::String& title,
                                     const juce::String& hint,
                                     const Priority p = Hover) {
        jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());
        const int newId = nextId++;
        entries.push_back({ newId, p, { title, hint } });
        std::stable_sort(entries.begin(), entries.end(),
            [](const Entry& a, const Entry& b) { return a.priority > b.priority; });
        notify();
        return { this, newId };
    }

    // Set the lowest-priority fallback shown when no transient hints are active.
    // CALL ON MESSAGE THREAD ONLY.
    void setPersistentHint(const juce::String& title, const juce::String& hint) {
        jassert(juce::MessageManager::getInstance()->isThisTheMessageThread());
        persistentContent = { title, hint };
        notify();
    }

    // Thread-safe bridge for model/DSP callbacks.
    // Posts an Urgent hint to the message thread that auto-expires after durationMs.
    void setHintAsync(const juce::String& title, const juce::String& hint, int durationMs = 3000) {
        auto alive = aliveFlag;
        juce::MessageManager::callAsync([this, title, hint, durationMs, alive] {
            if (!alive->load()) return;
            auto handle = std::make_shared<HintHandle>(setHint(title, hint, Urgent));
            const auto& aliveRef = alive;
            juce::Timer::callAfterDelay(durationMs, [h = std::move(handle), aliveRef]() mutable {
                if (!aliveRef->load()) return;
                h.reset();
            });
        });
    }

    // Register a callback invoked whenever the active hint changes.
    // Typically called once by PluginEditor to wire the HintBar.
    void setCallback(std::function<void(const HintContent&)> cb) {
        callback = std::move(cb);
        notify();
    }

private:
    friend class HintHandle;

    void clearHint(int id) {
        entries.erase(
            std::remove_if(entries.begin(), entries.end(),
                [id](const Entry& e) { return e.id == id; }),
            entries.end());
        notify();
    }

    void notify() const {
        if (callback)
            callback(entries.empty() ? persistentContent : entries.front().content);
    }

    struct Entry {
        int id;
        Priority priority;
        HintContent content;
    };

    std::vector<Entry> entries;   // maintained sorted: highest priority first
    HintContent persistentContent;
    int nextId = 0;
    std::function<void(const HintContent&)> callback;
    std::shared_ptr<std::atomic<bool>> aliveFlag;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(HintManager)
};
