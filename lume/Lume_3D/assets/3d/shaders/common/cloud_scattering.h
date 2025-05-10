#ifndef CLOUDS_SCATTERING_H
#define CLOUDS_SCATTERING_H

ScatteringParams DefaultParameters()
{
    return ScatteringParams(DEFAULT_PLANET_POS, // position of the planet
        DEFAULT_PLANET_RADIUS,                  // radius of the planet in meters
        DEFAULT_ATMOS_RADIUS,                   // radius of the atmosphere in meters
        DEFAULT_RAY_BETA,                       // Rayleigh scattering coefficient
        DEFAULT_MIE_BETA,                       // Mie scattering coefficient
        DEFAULT_ABSORPTION_BETA,                // Absorbtion coefficient
        DEFAULT_AMBIENT_BETA, // ambient scattering, turned off for now. This causes the air to glow a bit when no light
                              // reaches it
        DEFAULT_G,            // Mie preferred scattering direction
        DEFAULT_HEIGHT_RAY,   // Rayleigh scale height
        DEFAULT_HEIGHT_MIE,   // Mie scale height
        DEFAULT_HEIGHT_ABSORPTION,  // the height at which the most absorption happens
        DEFAULT_ABSORPTION_FALLOFF, // how fast the absorption falls off from the absorption height
        DEFAULT_PRIMARY_STEPS,      // steps in the ray direction
        DEFAULT_LIGHT_STEPS);
}

vec3 CalculateScatteringComposite(ScatteringResult result, vec3 scene_color)
{
    return result.ray + result.mie + result.ambient + scene_color * result.opacity;
}

vec3 ScatterTonemapping(vec3 col)
{
    return 1.0 - exp(-col);
}

// https://github.com/mxcop/src-dgi/blob/main/assets/shaders/shared/sky.slang
// float hash12(vec2 p)
//{
//    vec3 p3 = fract(vec3(p.xyx) * .1031);
//    p3 += dot(p3, p3.yzx + 33.33);
//    return fract((p3.x + p3.y) * p3.z);
//}
//
// float hash13(vec3 p3)
//{
//    p3 = fract(p3 * .1031);
//    p3 += dot(p3, p3.zyx + 31.32);
//    return fract((p3.x + p3.y) * p3.z);
//}
//
// float CalculateStars(vec3 rd)
//{
//    if (rd.y <= 0.0)
//        return 0.0;
//
//    float stars = 0.0;
//    vec2 uv = rd.xz / (rd.y + 1.0);
//
//    vec2 id = floor(uv * 700.0 + 234.0);
//    float star = hash12(id);
//    float brightness = smoothstep(0.98, 1.0, star);
//    stars += brightness * 0.5;
//
//    return stars * rd.y;
//}

vec3 CalculateStars(vec3 dir)
{
    vec3 star = vec3(noise(dir.xzy * 400)) * 0.3 - 0.5;
    star = min(vec3(0.8), star);
    star -= vec3(noise(dir.xzy * 200)) * 0.8;
    star += vec3(noise(dir.xzy * 150)) * 0.5;
    star = min(vec3(0.8), star);
    star *= -1;
    star = clamp(star, 0, 1);
    star = 1 - star;

    star = vec3(pow(star.x, 40));
    star = min(vec3(0.8), star);
    star *= 3;

    float n = noise(dir.xzy * 20);
    return star.x * (vec3(min(1, n * 2.0 - 1.0), 0, min(n, 1)) * 0.6 + vec3(1, 1, 1) * 0.4);
}

vec2 ComputePhase(float g, vec3 dir, vec3 l)
{
    float mu = dot(dir, l);
    float mumu = mu * mu;
    float gg = g * g;
    float phase_ray = 3.0 / (50.2654824574 /* (16 * pi) */) * (1.0 + mumu);
    float phase_mie = 3.0 / (25.1327412287 /* (8 * pi) */) * ((1.0 - gg) * (mumu + 1.0)) /
                      (pow(1.0 + gg - 2.0 * mu * g, 1.5) * (2.0 + gg));
    return vec2(phase_ray, phase_mie);
}

vec4 ComputeRaySphereCoeff(vec3 dir, vec3 start, float ds2)
{
    float a = dot(dir, dir);
    float b = 2.0 * dot(dir, start);
    float c = dot(start, start) - (ds2 * ds2);
    float d = (b * b) - 4.0 * a * c;
    return vec4(a, b, c, d);
}

vec3 ComputeDensity(float size, float absorption, float height, float falloff, vec2 scale)
{
    vec3 density = vec3(exp(-height / scale), 0.0);
    float denom = (absorption - height) / falloff;
    density.z = (1.0 / (denom * denom + 1.0)) * density.x;
    density *= size;
    return density;
}

float ComputeShadow(vec3 N, vec3 V, vec3 L)
{
    float dotNV = max(1e-6, dot(N, V));
    float dotNL = max(1e-6, dot(N, L));
    float shadow = dotNL / (dotNL + dotNV);
    return shadow;
}

void CalculateScatteringComponents(
    in Camera camera, in ScatteringLightSource light, in ScatteringParams params, out ScatteringResult result)
{
    vec3 start = camera.start - params.planetPosition;
    vec3 sumRay = vec3(0.0); // for rayleigh
    vec3 sumMie = vec3(0.0); // for mie

    vec4 intersectCoeff = ComputeRaySphereCoeff(camera.dir, start, params.atmoRadius);

    if (intersectCoeff.w < 0.0) {
        result.ray = vec3(0, 0, 0);
        result.mie = vec3(0, 0, 0);
        result.ambient = vec3(0, 0, 0);
        result.opacity = vec3(1, 1, 1);
        return;
    };

    vec2 rayLength = vec2(max((-intersectCoeff.y - sqrt(intersectCoeff.w)) / (2.0 * intersectCoeff.x), 0.0),
        min((-intersectCoeff.y + sqrt(intersectCoeff.w)) / (2.0 * intersectCoeff.x), camera.maxDist));

    if (rayLength.x > rayLength.y) {
        result.ray = vec3(0, 0, 0);
        result.mie = vec3(0, 0, 0);
        result.ambient = vec3(0, 0, 0);
        result.opacity = vec3(1, 1, 1);
        return;
    }

    // make sure the ray is no longer than allowed
    rayLength.y = min(rayLength.y, camera.maxDist);
    rayLength.x = max(rayLength.x, 0.0);

    float stepSizeP = (rayLength.y - rayLength.x) / float(params.numStepsPrimary);
    float rayPosP = rayLength.x + stepSizeP * 0.3;

    vec3 opticalDepthP = vec3(0.0);
    vec3 opticalDepthL = vec3(0.0);

    vec2 scaleHeight = vec2(params.heightRay, params.heightMie);
    vec2 phase = ComputePhase(params.g, camera.dir, light.dir) * vec2(1, camera.maxDist > rayLength.y ? 1 : 0);

    // raymarch along the camera direction
    [[unroll]] for (int p = 0; p < params.numStepsPrimary; ++p, rayPosP += stepSizeP) {
        const vec3 posP = start + camera.dir * rayPosP;
        const float heightP = length(posP) - params.planetRadius;
        const vec3 density =
            ComputeDensity(stepSizeP, params.heightAbsorption, heightP, params.absorptionFalloff, scaleHeight);
        opticalDepthP += density;

        // raymarch towards the light source
        intersectCoeff = ComputeRaySphereCoeff(light.dir, posP, params.atmoRadius);
        const float stepSizeL =
            (-intersectCoeff.y + sqrt(intersectCoeff.w)) / (2.0 * intersectCoeff.x * float(params.numStepsLight));
        float rayPosL = stepSizeL * 0.3;
        opticalDepthL = vec3(0.0);

        [[unroll]] for (int l = 0; l < params.numStepsLight; ++l, rayPosL += stepSizeL) {
            const vec3 posL = posP + light.dir * rayPosL;
            const float heightL = length(posL) - params.planetRadius;
            opticalDepthL +=
                ComputeDensity(stepSizeL, params.heightAbsorption, heightL, params.absorptionFalloff, scaleHeight);
        }

        opticalDepthL += opticalDepthP;
        const vec3 transmission = exp(-params.betaRay * opticalDepthL.x - params.betaMie * opticalDepthL.y -
                                      params.betaAbsorption * opticalDepthL.z);
        sumRay += density.x * transmission;
        sumMie += density.y * transmission;
    }

    const vec3 transmission = exp(-(
        params.betaMie * opticalDepthP.y + params.betaRay * opticalDepthP.x + params.betaAbsorption * opticalDepthP.z));
    result.ray = phase.x * params.betaRay * sumRay * light.intensity;
    result.mie = phase.y * params.betaMie * sumMie * light.intensity;
    result.ambient = opticalDepthP.x * params.betaAmbient * light.intensity;
    result.opacity = transmission;
}

vec3 CalculateScatteringSkylight(
    vec3 samplePos, vec3 surfaceNormal, vec3 backgroundCol, in ScatteringLightSource light, in ScatteringParams params)
{
    // slightly bend the surface normal towards the light direction
    surfaceNormal = normalize(mix(surfaceNormal, light.dir, 0.6));
    ScatteringResult result;
    CalculateScatteringComponents(Camera(samplePos,             // the position of the camera
                                      surfaceNormal,            // the camera vector (ray direction of this pixel)
                                      3.0 * params.atmoRadius), // max dist, essentially the scene depth
        light, params, result);

    return CalculateScatteringComposite(result, backgroundCol);
}
#endif // CLOUDS_SCATTERING_H
