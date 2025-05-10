#ifndef SHADERS_COMMON_CLOUD_STRUCTS_H
#define SHADERS_COMMON_CLOUD_STRUCTS_H

#ifdef VULKAN
struct UniformStruct {
    vec4 params[7];
};

struct VolumeSample {
    float transmission;
    float intensity;
    float ambient;
    float depth;
};

struct VolumeParameters {
    vec3 center;
    float radius;
    float nativeDepth;
    bool baseOnly;
    int minSamples;
    int maxSamples;
    bool conesampler;
    int volumeType;
};

struct Ray {
    vec3 o;
    vec3 dir;
};

struct ScatteringResult {
    vec3 mie;
    vec3 ray;
    vec3 ambient;
    vec3 opacity;
};

struct ScatteringLightSource {
    vec3 dir;
    vec3 intensity;
};

struct ScatteringParams {
    vec3 planetPosition; // the position of the planet
    float planetRadius;   // the radius of the planet
    float atmoRadius;     // the radius of the atmosphere
    vec3 betaRay;         // the amount rayleigh scattering scatters the colors (for earth: causes the blue atmosphere)
    vec3 betaMie;         // the amount mie scattering scatters colors
    vec3 betaAbsorption;  // how much air is absorbed
    vec3 betaAmbient; // the amount of scattering that always occurs, cna help make the back side of the atmosphere a
                      // bit brighter
    float
        g; // the direction mie scatters the light in (like a cone). closer to -1 means more towards a single direction
    float heightRay;         // how high do you have to go before there is no rayleigh scattering?
    float heightMie;         // the same, but for mie
    float heightAbsorption;  // the height at which the most absorption happens
    float absorptionFalloff; // how fast the absorption falls off from the absorption height
    int numStepsPrimary;     // the amount of steps along the 'primary' ray, more looks better but slower
    int numStepsLight;       // the amount of steps along the light ray, more looks better but slower
};

struct Camera {
    vec3 start;
    vec3 dir;
    float maxDist;
};
#endif // VULKAN
#endif // SHADERS_COMMON_CLOUD_STRUCTS_H