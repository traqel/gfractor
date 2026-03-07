#!/bin/bash
# Warn if potentially unsafe code patterns are added to processBlock

file="$TOOL_INPUT_file_path"

if grep -n 'processBlock' "$file" | head -1 | cut -d: -f1 | xargs -I {} awk 'NR >= {} && NR <= {}+100' "$file" | grep -E '(new |delete |malloc|free|std::vector.*push_back|std::lock|mutex)' > /dev/null; then
    echo '⚠️  Warning: Potential realtime-unsafe code detected in processBlock'
    echo 'Check for: allocations (new/delete/malloc), locks (mutex), or vector resizing'
    echo 'Review /juce-best-practices for realtime safety rules'
    exit 0
fi
