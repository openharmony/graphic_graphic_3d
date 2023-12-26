# AGP Engine

- Introduction
- Directory Structure
- Compilation and Building
- Description
    - Usage
- Repositories Involved

## Introduction
The AGP engine is Huawei's self-developed 3D rendering engine, which is an important part of the Graphic module of the operating system, shielding the differences between different GPU hardware drivers and different rendering backends, and providing basic rendering APIs and 3D rendering capabilities. As a software framework, the main function of the AGP rendering engine is to create, render, and manage 3D worlds, and modules such as plug-in system, rendering component, GPU resource management component, and ECS component system are all key parts of the AGP engine. The APG engine-based 3D widget is the basic component of ArKUI, which provides the ability to load models, animate playback, customize lights, cameras, and textures for developers to customize 3D models.
## Directory Structure

```
foundation/graphic/graphic_3d/
├── 3d_widget_adapter                  # Interface code with ArkUI
├── lume                               # Engine core code
│   ├── Lume_3D                        # Engine 3D rendering code
│   ├── LumeBase                       # The underlying library code of the engine
│   ├── LumeEngine                     # The engine loads plug-ins and resource management code
│   └── LumeRender                     # The engine renders the backend code
│   └── LumeBianryCompile              # Engine shader compilation
```
## Compilation and Building
```
# Through GN compilation, the lib3dWidgetAdapter.z.so, libAGPDLL.z.so, libPluginAGP3D.z.so, and libPluginAGPRender.z.so are generated in the folder of the corresponding product in the out directory
hb build graphic_3d
```
## Description
AGP Engine As a component of ArkUI, it provides the 3D drawing capability of the system
### Usage
For more information, please refer to the use of other components in ArkUI. The AGP engine mainly provides 3D painting capabilities, including engine loading, custom lighting, camera, and texture capabilities, for developers to customize 3D models.
## Repositories Involved
- [**graphic_graphic_2d**](https://gitee.com/abbuu_openharmony/graphic_graphic_2d)
- [arkui_ace_engine](https://gitee.com/openharmony/arkui_ace_engine)
- [third_party_egl](https://gitee.com/openharmony/third_party_egl)
- [third_party_opengles](https://gitee.com/openharmony/third_party_opengles)
- [third_party_skia](https://gitee.com/openharmony/third_party_skia)
