#version 450

layout(location = 0) in vec3 positions;
layout(location = 1) in vec3 colors;
layout(location = 2) in vec3 normals;
layout(location = 3) in vec2 uv;

layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec3 fragPosWorld;
layout(location = 2) out vec3 fragNormalWorld;

layout(push_constant) uniform Push { // There must be  no more than one push constant block statically used per shader entry point. 
	mat4 modelMatrix; 
	mat4 normalMatrix;
	// vec3 color; // Deprecated
} push; 

layout(set = 0, binding = 0) uniform GlobalUbo { 
	mat4 projectionViewMatrix;
	vec4 ambientLightColor;
	vec3 lightPosition;
	vec4 lightColor;
} ubo;

const vec3 DIRECTION_TO_LIGHT = normalize(vec3(1.0, -3.0, -1.0));
const float AMBIENT = 0.02;
void main() {
	
	vec4 positionWorld = push.modelMatrix * vec4(positions, 1.0);

	gl_Position = ubo.projectionViewMatrix * positionWorld;
	
	// Only works correctly if scale is uniform (sx == sy == sz)
	// vec3 normalWorldSpace = normalize(mat3(push.modelMatrix) * normals); // casting from mat4 to mat3 causes the 4th column and row to be stripped.

	// Calculating the inverse in a shader can be expensive and should be avoided
	// mat3 normalMatrix = transpose(inverse(mat3(push.modelMatrix)));
	// vec3 normalWorldSpace = normalize(normalMatrix * normal);

	// (modelMatrix^-1)^T = ((TRS)^-1)^T Normals not affected by Translation
	// ((RS)^-1)^T;			(AB)^-1 = B^-1 A^-1
	// (S^-1 R^-1)^T;		(AB)^T = B^T A^T
	// (R^-1)^T (S^-1)^T;	R^-1 = R^T For rotation matrix
	// (R^T)^T (S^-1)^T;	S^-1 is Diagonal Matrix so, D^T = D
	// (R^T)^T S^-1;		(A^T)^T = A
	// ----------
	// Therefore: (modelMatrix^-1)^T = RS^-1

	fragNormalWorld = normalize(mat3(push.normalMatrix) * normals);
	fragPosWorld = positionWorld.xyz;
	fragColor = colors;
}
