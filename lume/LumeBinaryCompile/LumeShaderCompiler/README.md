# LumeShaderCompiler

LumeShaderCompiler is a shader compiler for preprocessing shaders used with LumeEngine. Source GLSL files are compiled into SPIR-V, optionally optimized, and stored as `<input>.spv`. The SPIR-V is reflected and reflection stored as `<input>.lsb`.

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

# LumeSnippetCompiler

LumeSnippetCompiler generates GLSL files based on shader template and snippet blocks. The tool loads shader configuration files from the `source` path and writes GLSL files to `destination` path.

## Shader configuration
A shader configuration file is a JSON file containing:
```
{
    "target" : "<output file name .frag>"
    "template" : "<shader template .json>"
    "types": [<array of objects>]
    "uniforms" : [<array of objects>]
    "blocks" : [<array of objects>]
}
```
### `target`
Final shader will be written to `target`. The template used for creating the final shader is specified by `template`.

### `types`
Each custom struct in the `types` array is defined as follows:
```
{
    "name" : "<name of the struct type>"
    "members" : [
        {
            "type" : "<GLSL type of the struct member>"
            "name" : "<name of the struct member>"
        }
    ]
}
```

### `uniforms`
Each uniform variable in the `uniforms` array is defined as follows:
```
{
    "type" : "<GLSL type of the uniform variable>"
    "name" : "<name of the uniform variable>"
}
```

### `blocks`
A snippet block in the `blocks` array is defined as follows:
```
{
    "name" : "<identifier where to inject in the shader template>"
    "function" : "<name of the function to call>"
    "inputs" : [<array of objects>]
    "outputs" : [<array of objects>]
}
```
`name` should match one of the names listed in `//>DECLARATIONS_<name list>` and `//>FUNCTIONS_<name list>` tag comments in the template.

Each parameter in the `inputs` and `outputs` arrays is defined as follows:
```
{
    "type" : "<GLSL type of the variable passed as a parameter>"
    "name" : "<name of the variable passed as a parameter, optional for inputs with value>"
    "value" : "<optional intial value>"
}
```
Parameters are expected to be defined either in the template config, or uniform variable list.

## Template configuration
A template configuration file is a JSON file containing:
```
{
    "template" : "<shader template .h>"
    "pipelineLayout": "<pipline layout .json>",
    "types": [<array of objects>]
    "variables": {
        "inputs" : [<array of objects>]
        "outputs" : [<array of objects>]
    },
    "templateBlocks": [<array of objects>]
}
```

### `template`
Shader template is a GLSL shader with specific tag comments: `//>DECLARATIONS_<name list>` and `//>FUNCTIONS_<name list>`. The tool injects uniforms and function definitions above the maching `DECLARATIONS` tag and function calls above the matching `FUNCTIONS` tag in the order they appear in the configuration file. The applicable snippet block groups names are separated by whitespaces.

### `pipelineLayout`
The pipeline layout JSON defines the descriptor set bindings for the shader. If a pipeline layout is given for the template, any uniform variables introduced in the shader configuration will be added to the next free descriptor set. If layout was not defined, descriptor set 0 will be used. Pipeline layout for the shader configuration will be written in `<target>_pipeline.json` with the original file extension removed.

### `types`
The `types` array follows the same structure as in the [shader configuration](#types) JSON.

### `variables`
Each variable in `variables.inputs` and `variables.outputs` is defined as follows:
```
{
    "type" : "<GLSL type of the variable>"
    "name" : "<name of the variable>"
}
```

### `templateBlocks`
The `templateBlocks` array defines which snippet block groups can be used with the template. Group names are used to identify in which `//>DECLARATIONS_<name list>` and `//>FUNCTIONS_<name list>` the group can be used.
Each snippet block group in `templateBlocks` is defined as follows:
```
{
    "name" : "<name of the group>"
    "source" : "<snippet block group .json>"
}
```

## Snippet block group configuration
A snippet block group configuration file is a JSON file containing:
```
{
    "source" : "<snippet .h containing definitions of all the listed function snippets>"
    "blocks": [<array of objects>
```
A snippet block in the `blocks` array is defined as follows:
```
{
    "function" : "<name of the function to call>"
    "inputs" : [<array of objects>]
    "outputs" : [<array of objects>]
}
```

Each parameter in the `inputs` and `outputs` arrays is defined as follows:
```
{
    "type" : "<GLSL type of the variable passed as a parameter>"
    "description" : "<optional description of the parameter>"
}
```

## Commandline options:
`--source`, path where shader configuration files are located.

`--destination`, path where output is written. Source path is used if destination is not specified.

## TODO
- With current data some amount of type matching could be done.
- For a perfect type matching all includes would have to be resolved and GLSL parsed. This might help also variable definitions.
