#version 460

layout(set = 0, binding = 0) uniform sampler3D densityTex;
layout(set = 0, binding = 1) uniform sampler3D lightingTex;

layout(std140, binding = 3) uniform Consts {
    vec3 uvwOffset;
    float lightAbsorptionCoeff;
    vec3 lightDir;
    float densityAbsorption;
    vec3 lightColor;
    float aniso;
};

vec3 GetUVW(vec3 posModel) {
    return (posModel + 1.0) * 0.5;
}

float BeerLambert(float absorptionCoefficient, float distanceTraveled) {
    return exp(-absorptionCoefficient * distanceTraveled);
}

float HenyeyGreensteinPhase(vec3 L, vec3 V, float aniso) {
    float cosTheta = dot(L, -V);
    float g = aniso;
    return (1.0 - g * g) / (4.0 * 3.141592 * pow(abs(1.0 + g * g - 2.0 * g * cosTheta), 1.5));
}

layout(location = 0) in vec3 inPosModel; // Replace this with the actual input from your vertex shader
layout(location = 0) out vec4 outColor;

void main() {
    // vec3 eyeModel = mul(float4(eyeWorld, 1), worldInv).xyz; Replace with actual calculation
    vec3 eyeModel = vec3(0.0f);
    vec3 dirModel = normalize(inPosModel - eyeModel);

    int numSteps = 128;
    float stepSize = 2.0 / float(numSteps);

    vec3 volumeAlbedo = vec3(1, 1, 1);
    vec4 color = vec4(0, 0, 0, 1);
    vec3 posModel = inPosModel + dirModel * 1e-6;

    for (int i = 0; i < numSteps; ++i) {
        vec3 uvw = GetUVW(posModel);
        float density = texture(densityTex, uvw).r;
        float lighting = 1.0; // Or sample lightingTex if needed이거 강의 봐봐야야함

        if (density > 1e-3) {
            float prevAlpha = color.a;
            color.a *= BeerLambert(densityAbsorption * density, stepSize);
            float absorptionFromMarch = prevAlpha - color.a;

            color.rgb += absorptionFromMarch * volumeAlbedo * lightColor
                         * density * lighting
                         * HenyeyGreensteinPhase(lightDir, dirModel, aniso);
        }

        posModel += dirModel * stepSize;

        if (any(greaterThan(abs(posModel), vec3(1))))
            break;

        if (color.a < 1e-3)
            break;
    }

    color = clamp(color, 0.0, 1.0);
    color.a = 1.0 - color.a;
    outColor = color;
}