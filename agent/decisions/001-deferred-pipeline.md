# 001 — Baseline deferred pipeline

Date: 2026-11-20. Status: accepted.
Basis: explicit user choice while defining project goals.
Related milestones: M2–M4 in [Roadmap](../Roadmap.md).

## Context and choice

The goal is a basic engine emphasizing realistic rendering. Earlier alternatives
were forward PBR as an extension of the current pass and a true deferred renderer.
The user chose depth prepass → G-buffer pass → deferred lighting → postprocessing.

## Consequences

Do not substitute forward rendering for the selected baseline. Additional passes
for transparency or other needs are not prohibited.
Separate passes, multiple render targets, depth reads, G-buffer data contracts
and consistent resource states are required. Formats, depth conventions, BRDF,
alpha masking, light types and postprocess effects remain undefined.
C++/HLSL contracts are designed collaboratively; the user writes shaders with assistance.

## Verification and revisit conditions

Inspect intermediate buffers, resize and final images; measure pass costs.
A depth prepass is not a proven optimization without measurements.
Revisit on an explicit goal change or justified new requirements, recording a new
decision. Implementation is not yet confirmed.
