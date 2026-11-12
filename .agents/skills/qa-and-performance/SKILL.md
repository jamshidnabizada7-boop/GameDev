---
name: qa-and-performance
description: Use to test gameplay changes, investigate Unreal build or runtime errors, and protect FPS performance on a 16GB RAM RTX 3060 12GB computer.
---
# FPS QA and Performance
1. Record the exact game engine version, map and change being tested.
2. Test: project opens, Play-In-Editor launches, inputs work, no blocking error logs, target behaves as expected.
3. Monitor memory footprint, GPU VRAM usage and frame time; keep Scalable editor settings for initial iteration.
4. Never interpret missing errors as proof of correctness. State what was actually exercised.
5. When there is a crash or regression, capture the log, isolate the latest change and recommend a reversible fix.
6. Keep test results in `docs/testing/` where possible, including reproduction steps and pass/fail outcomes.
