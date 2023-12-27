# AGP Engine

## Introduction
The Ark Graphics Platform (AGP) engine is a cross-platform, high-performance real-time 3D engine that is easy to use, high-quality, and scalable. The engine is designed with an advanced Entity-Component-System (ECS) architecture and is modularly encapsulated (such as material definitions, post-processing effects, etc.), providing developers with a flexible and easy-to-use development kit. The AGP engine supports the Opengl ES/Vulkan backend to reduce developers' dependence on hardware resources.

The main structure of the AGP engine is shown in the following figure:

![AGP引擎架构图](./figures/graphic_3d_architecture.jpg)

The layering of the OpenHarmony 3D graphics stack is described as follows:

• Interface layer: provides graphical native API capabilities and ECS component systems.

• Engine layer

| Module                     | Competency description                                                                                       |
|------------------------|--------------------------------------------------------------------------------------------|
| Model parsing         | Provides the ability to parse GLTF models.                                                                               |
| Material Definitions            | Definitions for materials such as PBR (Physically Based Rendering) are provided.                               |
| animation           | Provides animation engine-related capabilities such as rigid bodies, bones, etc.                            |
| Lighting & Shadows & Reflections | Provide directional light, point light, spot light source and other light sources; Algorithms such as PCF (Hard Shadow-Based Anti-Aliasing Algorithm) are available.      |
| Post-processing effects | It mainly completes post-processing effects such as ToneMapping, Bloom, HDR (High Dynamic Range Imaging), FXAA (Fast Approximate Anti-Aliasing), and Blur.      |
| Plug-in system                | Provides the ability to load various plugins and develop new features with plugins. |
| resource management                | This module provides resource management capabilities, including memory management, thread management, and GPU resource management. |
| System abstraction                | It mainly includes file system, window system, debugging system, etc. |

• Graphical backend: OpenGL ES, Vulkan backend supported.

## Directory Structure

```
foundation/graphic/graphic_3d/
├── 3d_widget_adapter                  # Adapt to the ArkUI interface code
├── lume                               # Engine core code
│   ├── Lume_3D                        # ECS framework, 3D model analysis, and 3D rendering basic capabilities
│   ├── LumeBase                       # Basic data types, math libraries
│   ├── LumeEngine                     # Resource management, thread management, cross-platform, plug-in system
│   └── LumeRender                     # Engine backend, render pipeline
│   └── LumeBinaryCompile              # Engine shader compilation
```
## Compilation and Building
Through GN compilation, the lib3dWidgetAdapter.z.so, libAGPDLL.z.so, libPluginAGP3D.z.so, and libPluginAGPRender.z.so are generated in the folder of the corresponding product in the out directory.
```
hb build graphic_3d
```
## Description
AGP Engine As a component of ArkUI, it provides the 3D drawing capability of the system.
### Usage
For more information, please refer to the use of other components in ArkUI. The AGP engine mainly provides 3D painting capabilities, including engine loading, custom lighting, camera, and texture capabilities, for developers to customize 3D models.
## Repositories Involved
- [**graphic_graphic_2d**](https://gitee.com/abbuu_openharmony/graphic_graphic_2d)
- [arkui_ace_engine](https://gitee.com/openharmony/arkui_ace_engine)
- [third_party_egl](https://gitee.com/openharmony/third_party_egl)
- [third_party_opengles](https://gitee.com/openharmony/third_party_opengles)
- [third_party_skia](https://gitee.com/openharmony/third_party_skia)
