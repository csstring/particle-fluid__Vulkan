#version 460

layout(set = 2, binding = 0) uniform sampler3D densityTex;
layout(set = 2, binding = 1) uniform sampler3D lightingTex;

layout(push_constant) uniform Params {
    vec4 cursorPos;
    vec4 camPos;
    vec4 uvwOffset;
    vec4 lightDir;
    vec4 lightColor;
    float lightAbsorptionCoeff;
    float densityAbsorption;
    float aniso;
    float dt;
} params;

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
float sdSphere( vec3 p, float s )
{
  return length(p)-s;
}
float sdTorus( vec3 p, vec2 t )
{
  vec2 q = vec2(length(p.xz)-t.x,p.y);
  return length(q)-t.y;
}

float sdEllipsoid( vec3 p, vec3 r )
{
  float k0 = length(p/r);
  float k1 = length(p/(r*r));
  return k0*(k0-1.0)/k1;
}

layout(location = 0) in vec3 inPosModel;
layout(location = 1) in mat4 worldInv; 

layout(location = 0) out vec4 outColor;

void main() {

    vec3 eyeModel = (worldInv * vec4(params.camPos)).xyz;
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
        // float sdf1 = sdEllipsoid(posModel, vec3(0.8, .2, .2));
        // float sdf2 = sdTorus(posModel + vec3(0., 0.0,0.0), vec2(0.2, .7f));
        float sdf = sdSphere(posModel, 0.5);

        if (sdf > 0.0f){
            density *= clamp(1.0-sdf*2.0f, 0.0,1.0f);
        }
        if (density > 1e-3) {
            float prevAlpha = color.a;
            color.a *= BeerLambert(params.densityAbsorption * density, stepSize);
            float absorptionFromMarch = prevAlpha - color.a;

            color.rgb += absorptionFromMarch * volumeAlbedo * params.lightColor.xyz
                         * density * lighting
                         * HenyeyGreensteinPhase(params.lightDir.xyz, dirModel, params.aniso);
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