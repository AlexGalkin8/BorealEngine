# 003 — Centralized resource ownership

Date: 2026-11-20. Status: accepted.
Basis: the user confirmed the responsibility split and clarified that a resource
manager must create, retain and manage resources instead of transferring ownership
to consuming objects. Related work: [task 004](../tasks/004-render-resource-lifetime.md).

## Responsibilities

- Render device/backend provides low-level GPU resource creation.
- GPU resource manager coordinates creation, owns resource storage and controls
  lifetime. Consumers receive non-owning handles, pointers or references.
- Renderer and RenderMesh use resources and describe rendering relationships;
  they do not own mesh buffers, cached textures or render targets.
- Asset manager handles model/image loading, CPU caches and asset-to-GPU mappings.

unique_ptr may be used inside the owning manager. It must not be moved into
consumer objects as a workaround for missing lifetime management.
These responsibilities describe the intended architecture, not existing complete
implementations: the current scene ResourceManager only caches CPU model data.

## GPU lifetime

Distinguish logical release from physical destruction. Resource, descriptor and
upload memory reuse must respect completion of all queues that use them. Values
from independent queue fences are not interchangeable. Waiting for GPU idle is
acceptable during shutdown; it is not the intended per-resource release policy.

## Implementation details still open

Manager placement/API names, handle representation and invalidation, cache eviction,
asset-manager integration and fence retirement interfaces need concrete design.
A first implementation retaining resources until shutdown was proposed but is
not a completed implementation or a final dynamic-release design.

## Consequences and verification

Rework the provisional renderer-owned unique_ptr shutdown fix. Preserve valid
cleanup ordering and reporting changes. Verify shared resource use, shutdown,
subsequent resize/release scenarios and absence of premature GPU destruction.
Do not claim the revised ownership implemented until code and checks establish it.
