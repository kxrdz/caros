#version 110

varying vec3 normal;
varying vec3 pos;

uniform sampler2D diffusemap;
//uniform sampler2D shadowmap;

uniform vec4 specularColor;
uniform float smoothness;
uniform vec3 lightvector;
uniform vec4 lightpower;
uniform vec4 ambientColor;

uniform int reflectionMappingMethod;
uniform samplerCube reflectionMapCube;
uniform sampler2D reflectionMap2DSampler;
uniform vec3 reflectionMapStaticOffsetCoords;

vec4 reflectionMappingContribution(vec3 reflectDirection)
{
     vec3 envcolor = vec3(textureCube(reflectionMapCube, reflectDirection));
     return vec4(envcolor, 1.0);
}

vec3 fog_Func(vec3 color)
{
	const float LOG2 = 1.442695;
	float fogCoord = gl_ProjectionMatrix[3].z/(gl_FragCoord.z * -2.0 + 1.0 - gl_ProjectionMatrix[2].z);
	float fogFactor = exp2(-gl_Fog.density * gl_Fog.density * fogCoord * fogCoord * LOG2);

	if(gl_Fog.density == 1.0)
		fogFactor=1.0;

	return mix(gl_Fog.color.rgb, color, fogFactor);
}

void main()
{
    //vec3 delta_normal = texture2D(normalmap, vec2(gl_TexCoord[0]));
    //vec3 norma = normalize(normal+delta_normal);
    //vec3 s_worldspace = texture2D(shadowmap, vec2(gl_TexCoord[1]));
    vec3 norma = normal;
    vec3 v = normalize(-pos);
    vec3 l = normalize(lightvector);
    vec3 h = normalize(l+v);
    vec3 reflectDirection = reflect(v, normal);

    float cosTh = max(0.0, dot(norma, h));
    float cosTi = max(0.0, dot(norma, l));

    vec4 diffuse =  texture2D(diffusemap, vec2(gl_TexCoord[0]));
    //vec4 shadow =  texture2D(shadowemap, vec2(gl_TexCoord[1]));
    vec4 fcolor  =  ambientColor*diffuse+(diffuse+ smoothness*specularColor*pow(cosTh,smoothness))*lightpower*cosTi;
    vec4 final_color;

    if(reflectionMappingMethod!=0)
	{
        final_color = mix(reflectionMappingContribution(reflectDirection), fcolor , 0.70);
    }else
	{
        final_color = fcolor;
    }

	vec3 colorfog = vec3(final_color[0], final_color[1], final_color[2]);
    final_color = vec4(fog_Func(colorfog), final_color[3]);

    //final_color = (diffuse * shadow);
    gl_FragColor  = final_color;
}
