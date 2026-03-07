#!/bin/bash
# Block writes to processBlock if they contain memory allocations or locks

content="$TOOL_INPUT_content"

echo "$content" | grep 'processBlock' > /dev/null && \
echo "$content" | awk '/void.*processBlock/,/^}/' | grep -E '(\bnew\b|\bdelete\b|\bmalloc|\bfree|std::vector.*push_back|std::vector.*emplace|std::lock|std::mutex|std::unique_lock|std::lock_guard|jassert.*alloc)' > /dev/null && {
    echo '🚫 BLOCKED: Realtime-unsafe code detected in processBlock()'
    echo ''
    echo 'Violations found:'
    echo "$content" | awk '/void.*processBlock/,/^}/' | grep -n -E '(\bnew\b|\bdelete\b|\bmalloc|\bfree|std::vector.*push_back|std::vector.*emplace|std::lock|std::mutex|std::unique_lock|std::lock_guard)' | head -5
    echo ''
    echo 'Audio thread must not:'
    echo '  • Allocate memory (new, delete, malloc, free, vector::push_back)'
    echo '  • Use locks (mutex, lock_guard, unique_lock)'
    echo '  • Block or wait'
    echo ''
    echo 'Solutions:'
    echo '  • Pre-allocate buffers in prepareToPlay()'
    echo '  • Use lock-free data structures (juce::AbstractFifo)'
    echo '  • Move allocations to message thread'
    echo ''
    echo 'See /juce-best-practices skill for details'
    exit 1
} || exit 0
