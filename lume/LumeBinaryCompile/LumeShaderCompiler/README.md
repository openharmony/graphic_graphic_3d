# LumeShaderCompiler

LumeShaderCompiler is a shader compiler for preprocessing shaders used with LumeEngine. Source GLSL files are compiled
into SPIR-V, optionally optimized, and stored as `<input>.spv`. The SPIR-V is reflected and reflection stored
as `<input>.lsb`.

## Commandline options:

`--source`, path where source GLSL files are located.

`--destination`, path where output is written. Source path is used if destination is not specified.

`--include`, path where to look for include files. Multipla include paths can be given.

`--optimize`, generate optimized SPIR-V binary.

`--monitor`, keep monitoring the source path for file changes and recompile modified shaders.

# LSB format, version 0

## Header:

```
uint8[4] tag
uint16 offsetPushConstants
uint16 offsetSpecializationConstants
uint16 offsetDescriptorSets
uint16 offsetInputs
uint16 offsetLocalSize
```

tag[0] = 'r', tag[1] = 'f', tag[2] = 'l', tag[3] = 0

offsetPushConstants, if non-zero offset to PushConstants

offsetSpecializationConstants, if non-zero offset to SpecializationConstants

offsetDescriptorSets, if non-zero offset to DescriptorSets

offsetInputs, if non-zero offset to Inputs

offsetLocalSize, if non-zero offset to LocalSize

## PushConstants:

```
uint8 count
uint16[count] byteSizes
```

count, number of push constants

byteSizes[], size of push constant in bytes

## SpecializationConstants:

```
uint32 count
{
    uint32 id,
    uint32 byteSize
}[count] constants
```

count, number of specialization constants

constants[].id, ID of the constant

constants[].byteSize, size of push constant in bytes

## DescriptorSets:

```
uint16 count
{
    uint16 set,
    uint16 bindingCount
    {
        uint16 index,
        uint16 type,
        uint16 count
    }[bindingCount] bindings
}[count] sets
```

count, number of descriptor sets

sets[].set, ID of the descriptor set

sets[].bindingCount, number of descriptor bindings in the set

sets[].bindings[].index, index where resource is bound

sets[].bindings[].type, type of the bound resource

sets[].bindings[].count, number of resources

## Inputs:

```
uint16 count
{
    uint16 location,
    uint16 format
}[count] inputs
```

count, number of inputs

inputs[].location, location where input is bound

inputs[].format, expected input format (matches CORE_NS::Format)

## LocalSize:

```
uint32 localX
uint32 localY
uint32 localZ
```

localX, x dimension of shader execution local size

localY, y dimension of shader execution local size

localZ, z dimension of shader execution local size
