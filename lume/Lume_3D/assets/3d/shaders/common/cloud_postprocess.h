#ifndef CLOUDS_POSTPROCESS_H
#define CLOUDS_POSTPROCESS_H

#include "common/cloud_common.h"
#include "common/cloud_scattering.h"
#include "common/cloud_structs.h"

vec4 Postprocess(in Ray ray, in VolumeSample o, in int colorType)
{
    vec4 finalColor;

    // reconstruct the particle
    float d = o.depth <= 0.0 ? 1 : o.depth;
    vec3 p = ray.o + ray.dir * d;
    vec3 L = normalize(uSunPos() - p);
    vec3 V = normalize(p - ray.o);
    vec3 B = normalize(vec3(ray.dir.x, 0, ray.dir.z));
    float s = dot(L, normalize(B));

    vec3 planetHit =
        RaySphereIntersect(ray.o - DEFAULT_PLANET_POS, ray.dir, DEFAULT_PLANET_RADIUS - DEFAULT_PLANET_BIAS);
    float maxDepth =
        planetHit.z == 1.0 && planetHit.x >= 0 && planetHit.y >= 0 ? min(min(planetHit.x, planetHit.y), 1e12) : 1e12;
    vec3 normal = normalize(ray.o - DEFAULT_PLANET_POS + ray.dir * d);
    float NdotL = dot(normal, normalize(uSunPos()));

    float Cos_angle = dot(L, V);
    float phase = HenyeyGreensteinPhase(Cos_angle, 0.2, silverIntensity(), silverSpread());

    // calculate the skybox color, could do subpassLoads
    ScatteringResult scattSample;
    ScatteringResult scattLight;
    ScatteringResult scattCamera2;
    ScatteringResult scattReal;

    float max_atmos = DEFAULT_ATMOS_RADIUS * 3;

    float max_distance = 1e12;

    [[branch]] if (o.transmission == 1) {
        return vec4(0, 0, 0, 0);
    }
    [[branch]] if (o.depth <= 0) {
        return vec4(0, 0, 0, 0);
    }

    // Uncomment this to just return a flat shaded clouds
    // return vec4(1,0,0, 1 - o.transmission);

    if (planetHit.z == 1.0 && planetHit.x > 0)
        max_distance = min(max_distance, max(planetHit.x, 0.0));
    if (planetHit.z == 1.0 && planetHit.y > 0)
        max_distance = min(max_distance, max(planetHit.y, 0.0));
    float max_cloud_depth = o.depth <= 0.0 ? max_distance : min(max_distance, o.depth / scatteringDistance());

    ScatteringParams params = DefaultParameters();
    params.numStepsPrimary = 2;
    params.numStepsLight = 4;

    CalculateScatteringComponents(Camera(ray.o,         // the position of the camera
                                      ray.dir,          // the camera vector (ray direction of this pixel)
                                      max_cloud_depth), // max dist, essentially the scene depth
        ScatteringLightSource(normalize(uSunPos()),     // light direction
            vec3(40.0)),
        params, scattSample);

    params.numStepsPrimary = 2;
    params.numStepsLight = 4;

    CalculateScatteringComponents(Camera(ray.o + ray.dir * max_cloud_depth, // the position of the camera
                                      ray.dir, // the camera vector (ray direction of this pixel)
                                      max_distance - max_cloud_depth), // max dist, essentially the scene depth
        ScatteringLightSource(normalize(uSunPos()),                    // light direction
            vec3(40.0)),
        params, scattCamera2);

    params.numStepsPrimary = 2;
    params.numStepsLight = 4;

    CalculateScatteringComponents(Camera(ray.o,             // the position of the camera
                                      ray.dir,              // the camera vector (ray direction of this pixel)
                                      max_distance - 1000), // max dist, essentially the scene depth
        ScatteringLightSource(normalize(uSunPos()),         // light direction
            vec3(40.0)),
        params, scattReal);

    params.numStepsPrimary = 1;
    params.numStepsLight = 8;

    CalculateScatteringComponents(Camera(ray.o + ray.dir * d, // the position of the camera
                                      ray.dir,                // the camera vector (ray direction of this pixel)
                                      100),                   // max dist, essentially the scene depth
        ScatteringLightSource(normalize(uSunPos()),           // light direction
            vec3(40.0)),
        params, scattLight);

    const float cloudFadeOutPoint = 0.06f;
    float blend = smoothstep(0.0, 1.0, min(1.0, remap(ray.dir.y, cloudFadeOutPoint, 0.1f, 0.0f, 1.0f)));

    if (colorType == 0) {
        [[branch]] if (false) {
        }
#if 0
        else if( inUv.x < 0.1 ) 
        {
            if( inUv.y < 0.5 )
            {
                finalColor = vec4(o.ambient,0,0,1);      
            }
            else
            {
                finalColor = vec4(o.intensity,0,0,1);             
            }
        }
        else if( inUv.x < 0.2 ) 
        {
            if( inUv.y < 0.5 )
            {
                finalColor = vec4(o.transmission,0,0,1);
            }
            else
            {
                finalColor = vec4(1 - o.transmission,0,0,1);
            }
        }
        else if( inUv.x < 0.3 ) 
        {
            finalColor = vec4( vec3(1) * o.ambient , 1 - o.transmission); 
        }
        else if( inUv.x < 0.4 ) 
        {
            finalColor = vec4( vec3(1) * o.intensity , 1 - o.transmission); 
        }
#endif

#if 0
        else if( inUv.x < 0.45 ) 
        {
            finalColor = vec4(  scattCamera.mie, 1); 
        }
        else if( inUv.x < 0.475 ) 
        {
            finalColor = vec4(  scattCamera.ray, 1); 
        }
        else if( inUv.x < 0.5 ) 
        {
            finalColor = vec4(  scattSample.mie, 1); 
        }
        else if( inUv.x < 0.525 ) 
        {
            finalColor = vec4(  scattSample.ray, 1); 
        }
        else if( inUv.x < 0.55 ) 
        {
            finalColor = vec4(  scattSample.opacity, 1); 
        }
        else if( inUv.x < 0.60 ) 
        {
            finalColor = vec4(  scattLight.opacity, 1); 
        }
#endif
        else {

            float ambient_energy = 1;
            float direct_energy = 1;

            vec3 ambientColor = vec3(0, 0, 0);
            vec3 directColor = vec3(0, 0, 0);

            float ambient_term = 1;
            float direct_term = 1;

            float blend = mix(0.0, 0.08,
                clamp(Luminance(ScatterTonemapping(scattCamera2.mie + scattCamera2.ray + scattCamera2.ambient)), 0, 1));

            float int_ = 0.0;
            float x = Luminance(ScatterTonemapping(scattReal.mie + scattReal.ray + scattReal.ambient)) +
                      0.1 * Luminance(scattReal.opacity) + NdotL * 0.9 * Luminance(scattReal.opacity);
            x = clamp(x * 2.2, 0, 1);
            int_ = x * 0.02;

            directColor = scattReal.mie + scattReal.ray + scattReal.ambient;
            // if( isnan(directColor.x) ) directColor = vec3(0,0,0);

            directColor = ScatterTonemapping(directColor * (1 - o.transmission));
            directColor +=
                ScatterTonemapping((scattLight.mie + scattLight.ray + scattLight.ambient) * (o.transmission) +
                                   max(0.0015, NdotL) * (scattLight.opacity) * (o.transmission));
            directColor = (scattCamera2.mie + scattCamera2.ray + scattCamera2.ambient) * (1 / 160) * int_ +
                          directColor * scattCamera2.opacity;
            directColor = ScatterTonemapping(directColor);

            ambientColor = vec3(1, 1, 1);
            ambientColor = ScatterTonemapping((scattLight.mie + scattLight.ray + scattLight.ambient) +
                                              max(vec3(0.01, 0.01, 0.015), NdotL) * Luminance(scattLight.opacity));
            ambientColor = (scattCamera2.mie + scattCamera2.ray + scattCamera2.ambient) * (1 / 160) * int_ +
                           ambientColor * scattCamera2.opacity;
            ambientColor = ScatterTonemapping(ambientColor);
            ambientColor = ambientColor * mix(0.2, 0.8, phase);

            finalColor = vec4(ambientColor * o.ambient * ambientStrength() * ambient_term +
                                  directColor * o.intensity * direct_term * directStrength(),
                mix(1 - blend, 0, o.transmission));

            // apply scattering from the sample to the camera
            finalColor = vec4(ScatterTonemapping(scattSample.mie + scattSample.ray + scattSample.ambient) * 0.5 +
                                  finalColor.rgb * scattSample.opacity,
                finalColor.a);

            // apply scattering tone mapping
            finalColor = vec4(ScatterTonemapping(finalColor.rgb), finalColor.a);

            // finalColor = vec4( scatter_tonemapping( scattReal.composite.rgb) * 0.8 , 1 - (o.transmission - blend *
            // o.transmission) ); finalColor = vec4( mix(vec3(1,1,1), max(vec3(0), scattCamera.ray - scattSample.ray),
            // 0.6) * o.ambient * ambient_energy + max(vec3(0), scattCamera.mie - scattSample.mie) * o.intensity *
            // direct_energy, 1 - o.transmission ); finalColor = vec4( cam.rgb, 1 - o.transmission );

#if 0
            //finalColor = vec4(lum.xyz, 1 - o.transmission );
#endif

#if 0
            // use this to just have a simple ambient color and sun color
            finalColor = vec4( uAmbientColor() * o.ambient + uSunColor() * o.intensity, 1 - o.transmission );
#endif
        }

        [[branch]] if (isnan(o.transmission) || isinf(finalColor.x) || isinf(finalColor.y) || isinf(finalColor.z) ||
                       isinf(finalColor.w) || isnan(finalColor.x) || isnan(finalColor.y) || isnan(finalColor.z) ||
                       isnan(finalColor.w)) {
            finalColor = vec4(1, 1, 0, 1);
            finalColor = vec4(1, 0, 1, 1 - o.transmission);
        }
    } else {
#if 0
 
        // calculate the skybox color in the oposite direction
        vec4 skyColor1, skyColor3;
        vec3 skyAmbient1, skyRayleigh1, skyMie1;
        vec3 skyAmbient3, skyRayleigh3, skyMie3;



        Ray ray2;
        ray2.o = ray.o + reflect(V, vec3(0, 1, 0)) * -o.depth;
        ray2.dir = normalize(uSunPos() - ray2.o);
        mainImage3(skyColor1, ray2, skyRayleigh1, skyMie1, skyAmbient1);

        Ray ray3;
        ray3.o = ray.o;
        ray3.dir = reflect(V, vec3(0, 1, 0));
        mainImage3(skyColor3, ray3, skyRayleigh3, skyMie3, skyAmbient3);


        float density = o.intensity;
        float NdotL = dot(-V, L);
        float LdotU = dot(L, vec3(0,1,0));

 

        float intensity = o.intensity;
        float transmission = o.transmission;
        float absorption = (1 - o.transmission);


        float directLightIntensity = o.intensity * 0.7;
        float ambientLightIntensity = (1 - o.intensity) * 0.3;
        float blendFactor = 1.0;

        // vec3 litColor = skyRayleigh * directLightIntensity * intensity;
        // vec3 unlitColor = skyMie * ambientLightIntensity * intensity;
        // vec3 ambient = skyAmbient + skyMie * 40;
        // vec3 direct = skyRayleigh * 40;


        float gradientUp = max(0, LdotU);
        float gradientSide = mix(1, 0, (-V.y) * 0.5 + 0.5);

        vec3 albedo = mix(vec3(0.7), vec3(1.0), gradientUp * gradientSide  ) * mix(0.7, 1.0, absorption);
        vec3 diffuse = max(0, NdotL * 0.5 + 0.5) * albedo * mix(vec3(0), uSunColor(), transmission);
        vec3 specular = vec3(0,0,0);
        vec3 ambient = (skyAmbient + skyMie ) * albedo * 40;
        vec3 emmisive = max(0, NdotL * 0.5 + 0.5) * albedo;
        vec3 final = ambient + diffuse + specular + emmisive;
        //     final = vec3(transmission);
        //     final = albedo;


        /*
        float forward = mix(max(-s, 0), 0.3, 0.3);
        float backward = mix(max(s, 0), 0.3, 0.3);
        vec3 forwardLight = skyMie * 40 + skyRayleigh * forward * 40 * absorption;
        vec3 backwardLight = skyMie1 * 40 + skyRayleigh1 * backward * 40 * transmission;
                forwardLight = (skyMie + skyRayleigh)  * remap(s, -1, 1, 0, 1) * 40;
                backwardLight = (skyMie1 + skyRayleigh1) * remap(s, -1, 1, 1, 0) * 40;

        final = (skyMie * 40 + skyRayleigh * 40) * remap(s * absorption, -1, 1, 1, 0) + (skyMie * 40) * remap(s * absorption, -1, 1, 0, 1);
        final = final * remap(s, 1, -1, 1, 0.2);

        final = final * albedo * intensity;
        final = mix( final, final + skyAmbient, 0.1);
        */


        finalColor = vec4( mix(skyColor.xyz, final, absorption  ), blend);

#endif
    }

    return finalColor;
}

#endif // CLOUDS_POSTPROCESS_H
