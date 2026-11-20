# BorealEngine project brief

Updated: 2026-11-20, based on user decisions.

## Confirmed goal

Build a basic game engine supporting level creation and adding objects to levels.
The author is a Render Engineer; development emphasizes beautiful, realistic images
and modern rendering approaches. This long-term ambition does not require every
technology in the first milestone.

## First result

- Model loading, level creation and object placement.
- A minimal custom Dear ImGui editor.
- A complete baseline: depth prepass → G-buffer pass → deferred lighting → postprocessing.
- Material, light, transparency, shadow and postprocess details are defined in their tasks.

Accepted decisions: [pipeline](decisions/001-deferred-pipeline.md),
[editor](decisions/002-imgui-editor.md).

## Hardware and performance

Minimum hardware target: NVIDIA RTX 3060, Intel Core i7-8700K, 16 GB RAM.
Hardware constraints are not considered strict by the user. This is a target,
not a statement about the user's actual machine or measured engine performance.
Target resolution: 1920×1080 (1080p), 60 FPS, approximately 16.67 ms per frame.
Specific VRAM capacity, per-pass budgets and memory budgets remain undefined.
Define a reference scene and quality settings before validating performance.

## Collaboration

- Implement collaboratively and explain algorithms and approaches.
- The user writes shaders with agent assistance in design, explanation, debugging
  and review. Agent implementation of HLSL requires an explicit assignment;
  a general feature request does not cancel this division of work.
- There is no requirement to build everything from scratch. Select dependencies
  for concrete needs and explain their purpose; no general library list is approved.
- Work proceeds as time permits, without fixed deadlines or weekly hours.
- Store project context in the repository. New directories use lowercase.
- Comments, commit messages and documentation are in English; conversation is in Russian.
  Durable instructions are in the root AGENTS.md.

## Technical starting point

Source inspection on 2026-11-20 found C++20, Windows, D3D12, a Visual Studio solution,
tinygltf import, actors/components, an FPS camera and JSON scene descriptions.
This is a snapshot; check current sources. Selecting deferred rendering does not
mean it is implemented. The initial analysis did not confirm a successful build
or visual correctness.

## Open questions

| Question | When to resolve |
|---|---|
| CPU/GPU budget allocation, memory budgets and quality settings | Before performance validation |
| Reference scene scale, object count and light count | When preparing the test level |
| G-buffer contents/formats, depth convention and alpha masking | Before depth/G-buffer implementation |
| Material model, light types and postprocess effects | During lighting/postprocessing design |
| Transparency, shadows and additional passes | In separate tasks after baseline contracts |
| Level format and editor/runtime boundary | Before editor implementation |
| Vulkan, other platforms and advanced rendering priorities | After the baseline or on explicit request |

Do not invent unspecified parameters and present them as user requirements.
