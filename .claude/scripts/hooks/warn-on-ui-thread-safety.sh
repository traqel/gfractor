#!/bin/bash
# warn-on-ui-thread-safety.sh
# Warns if UI component methods are called from audio thread (processBlock)

set -e

FILE="$1"

if [ -z "$FILE" ]; then
    exit 0
fi

# Patterns that indicate potential UI thread safety issues
# These are calls that should only happen on the message thread, not audio thread

UNSAFE_PATTERNS=(
    "->repaint()"
    "->setVisible("
    "->setBounds("
    "->addChildComponent("
    "->removeChildComponent("
    "->setEnabled("
    "juce::MessageManager"
    "juce::Component::"
)

# Check if file is a processor that handles audio
if grep -q "processBlock" "$FILE" 2>/dev/null; then
    for pattern in "${UNSAFE_PATTERNS[@]}"; do
        if grep -q "$pattern" "$FILE" 2>/dev/null; then
            echo "⚠️  UI Thread Safety Warning: Found '$pattern' in $FILE"
            echo "   UI methods should not be called from processBlock (audio thread)."
            echo "   Use juce::MessageManager::callAsync() or atomic flags instead."
            echo ""
        fi
    done
fi
