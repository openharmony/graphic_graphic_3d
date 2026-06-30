# LumeTS AGENTS.md

This is the top-level monorepo for LumeTS — a 3D scene engine with N-API bindings for JavaScript/ArkTS.

## Submodule Documentation

Detailed knowledge docs live in `docs/knowledge/`:

| Scope | Document |
|-------|----------|
| ECS system & component authoring, component managers, systems, unit tests | [Lume3D.md](docs/knowledge/Lume3D.md) |
| Scene layer wrapping ECS into typed interfaces (components, nodes, property forwarding) | [LumeScene.md](docs/knowledge/LumeScene.md) |
| N-API bindings exposing LumeScene to JS/ArkTS (wrappers, proxies, threading) | [ScenePluginAddon.md](docs/knowledge/ScenePluginAddon.md) |

## Module Dependency Graph

The three modules documented under `docs/knowledge/` form a consumer chain
(each depends on the one below, plus lower-level engine/render/base libs
detailed in each module's own `## Module Dependencies` section):

```
ScenePluginAddon (scene3d.node) — N-API bindings exposing LumeScene to JS/ArkTS
  └── LumeScene — high-level scene interfaces (IScene, INode, ICamera)
        └── Lume3D — ECS components (Transform, Camera, Light), graphics context
```

Other top-level modules (no knowledge doc):
- `ModuleDeclaration` — public TypeScript/ArkTS API type declarations
- `ScenePluginTest` — integration tests for ScenePluginAddon (Windows TS app)
- `NativeDemo` — OHOS demo app exercising ScenePluginAddon
- `WindowAddon` — window management addon (`.node`)
- `SharedAddonCode` — shared source compiled into ScenePluginAddon & WindowAddon

## Build

### Windows

```powershell
build.bat vs2022     # VS 2022, Debug
build.bat vs2017     # VS 2017, Debug
```

### On Linux Build OHOS

```powershell
# Ensure in dir <prefix>/code
./build_system.sh --abi-type generic_generic_arm_64only --device-type general_all_phone_standard --ccache --build-target graphic_3d graphic_3d_ext --build-variant root
```

## Boundaries

### Always

- **Compile-test after changes.** After any modification to source files in this repo or its submodules, run `build.bat` and fix any compilation errors before considering the task complete. Use a subagent to execute the build and resolve errors iteratively.

### Ask First

- **Which module should be modified?** Confirm the target module before making changes. The dependency graph above shows the allowed direction — avoid modifying lower-level modules (LumeBase, LumeEngine) without explicit approval.

### Never

- Do not modify files under `submodules/` unless explicitly asked — these are versioned separately.
