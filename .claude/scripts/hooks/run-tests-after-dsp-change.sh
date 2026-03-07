#!/bin/bash
# Run DSP tests after modifying DSP source files

if [ -f "build/Tests/DSPTests" ] || [ -f "build/Tests/RunUnitTests" ]; then
    echo '🧪 Running DSP tests after code change...'
    (cd "$CLAUDE_PROJECT_DIR" && ctest --test-dir build -R DSP --output-on-failure) && echo '✅ DSP tests passed' || echo '⚠️  DSP tests failed - review output above'
else
    echo 'ℹ️  DSP tests not found - run cmake to configure tests'
fi
