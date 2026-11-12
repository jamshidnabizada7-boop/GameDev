---
name: blender-to-unreal
description: Use when preparing, exporting, importing and verifying Blender meshes or animations for Unreal Engine 5.
---
# Blender to Unreal Asset Pipeline
1. Check the Blender and Unreal versions and choose a compatible interchange route (FBX or glTF as supported by the project).
2. Verify scale, origins, transforms, normals, UVs and texture maps; identify collision requirements.
3. Export one test asset first, then import into a dedicated Unreal asset folder.
4. Open imported content in Unreal and check scale, orientation, materials, collisions and LOD suitability.
5. Document export/import settings used so other assets are reproducible.
6. If using an automation addon, verify compatibility with the installed Blender and Unreal versions before enabling it.
