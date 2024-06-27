# Athena
Athena is an early-stage Game Engine with a high emphasis on Rendering.     
Currently supports only Windows platform and Visual C++. Vulkan API - the only way to go!    
Expect bugs and instability as it is still under development. The engine is free and open source.    
Feel free to contact me about any questions or issues you have!

# Screenshots

# MainFeatures
![Editor1](https://github.com/Algor1tm/Athena/assets/68811145/73d81214-1bfe-4be9-a4c1-ffbe234911fb)

![Sponza2](https://github.com/Algor1tm/Athena/assets/68811145/ed4898d1-e80e-42db-a7a3-c0f16adda1f0)

![GBuffer](https://github.com/Algor1tm/Athena/assets/68811145/08a3b7b3-1c85-412f-954a-1a9f4c4ca742)

### Rendering

- Deferred Renderer
- PBR, IBL, HDR
- 2.5D Light Culling
- Atmosphere Rendering
- PCSS Cascaded Shadows
- Bloom
- Skeletal Animations
- Instancing
- HBAO+
- HiZ SSR
- Jump Flood outline
- Font Rendering
- SMAA 1x, FXAA

## Engine

- Entity Component System(ECS)
- Event system
- Python scripting language
- Editor with docking support
- File dialogs, material & component editors
- Basic Runtime
- One-click scene loading/saving, YAML serializer(and deserializer) for all components and resources
- Logging, image & cubemap loading via editor, YAML reading, UUID, random generators and many more!

# Plans

- Asset System
- Physics
- Transparency
- Render Graph
- Async Compute


# Getting Started
Visual Studio 2019 or 2022 is recommended.  
For compiling the project you need to have Python 3.7+ and Vulkan SDK with VMA and shader toolchain debug symbols (x64).    
To setup projects run one of the scripts in <b>Scripts</b> folder.      
You will find solution file(.sln) in <b>Build/Projects</b> folder.      
