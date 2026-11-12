---
name: unreal-blueprints
description: Use for creating or modifying Unreal Engine 5 Blueprint gameplay mechanics, inputs, targets, objectives or small FPS prototype systems.
---
# Unreal Blueprint Gameplay
1. Inspect the `.uproject`, engine version, plugins, and existing assets before proposing changes.
2. Prefer existing First Person template classes and Enhanced Input mapping; do not recreate a system that already works.
3. Specify exact asset names, folder paths, event graphs or editor actions when direct MCP access is unavailable.
4. Add one mechanic per iteration and test it in Play-In-Editor before proceeding.
5. If modifying Blueprints using an MCP bridge, verify changed objects and save the assets; don't assume a tool call succeeded.
6. Summarize functionality, controls, expected outcome, evidence from the test, and unresolved errors.
