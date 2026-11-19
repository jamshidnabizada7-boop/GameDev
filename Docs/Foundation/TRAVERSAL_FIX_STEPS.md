# G TRAVERSAL FIX STEPS
## Batch 2R Item: Fix G key vault/climb

**Problem found:**
1. G key NOT bound in IMC_Default
2. EventGraph has NO IA_Traversal input node
3. TryTraversal function exists but never called

**Fix steps:**

### A. Bind G key to IA_Traversal in IMC_Default

1. Open `Content/AfghanFPS/Input/IMC_Default`
2. Add mapping:
   - Input Action: `IA_Traversal`
   - Key: **G**
   - Modifiers: none
   - Triggers: Pressed
3. Save

### B. Wire IA_Traversal to TryTraversal in BP_PlayerFoundation EventGraph

1. Open `BP_PlayerFoundation` → EventGraph
2. Right-click → Input Action → **IA_Traversal (Enhanced Input)**
3. Drag from **Started** exec pin
4. Add node: **Call TryTraversal** (custom function)
5. Wire: `IA_Traversal Started` → `TryTraversal`
6. Compile

### C. Verify TryTraversal logic (optional inspection)

The function has:
- Forward line trace
- Upward trace for ledge
- Height check
- LaunchCharacter node
- SetActorLocation node

If G still doesn't work after A+B, inspect:
- Trace distances (should be ~200-300 cm forward)
- Collision channels (must see WorldStatic)
- Height thresholds
- Landing clearance check

### D. Test in PIE

1. Place test obstacles in L_Test_PlayerFoundation:
   - Low box (~100 cm high)
   - Ledge platform (~160 cm high)
2. PIE → press G at obstacles
3. Expected: vault over low, climb up ledge

**Status:** Manual editor steps required (headless UE cannot create graph nodes or bind IMC mappings)
