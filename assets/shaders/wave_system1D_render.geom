#version 450 core 

layout (points) in;
layout (triangle_strip, max_vertices = 24) out; // 6 faces * 4 triangles per face 

layout (location = 0) in VS_GS_VERTEX 
{
	float dx;
	float velocity;
} vertex_in[];

layout (location = 0) uniform mat4 uProjectionMatrix;
layout (location = 1) uniform mat4 uViewMatrix;
layout (location = 2) uniform mat4 uModelMatrix;
layout (location = 3) uniform float uBoxPercentage;  // percentage of the dx space to make this box

//const mat4 modelViewMatrix = uViewMatrix * uModelMatrix;
//const mat4 mvpMatrix = uProjectionMatrix * modelViewMatrix;
//const mat4 viewProjectionMatrix = uProjectionMatrix * uViewMatrix;
//const mat3 normalMatrix = mat3(transpose(inverse(modelViewMatrix)));

// unit cube
const vec4 CubePoints[8] = 
{ 
	// Back
	vec4(0.0, 0.0, 0.0, 1.0), // 0  
	vec4(1.0, 0.0, 0.0, 1.0), // 1
	vec4(0.0, 1.0, 0.0, 1.0), // 2
	vec4(1.0, 1.0, 0.0, 1.0), // 3
	// Front
	vec4(0.0, 0.0, 1.0, 1.0), // 4
	vec4(1.0, 0.0, 1.0, 1.0), // 5
	vec4(0.0, 1.0, 1.0, 1.0), // 6
	vec4(1.0, 1.0, 1.0, 1.0)  // 7
};

struct CubeFace 
{
	vec4 p[4];
	vec3 n;
};

layout (location = 0) out GS_FS_VERTEX 
{
	vec3 normal;
	vec3 worldPosition;
	float velocity;
} vertex_out;

mat4 getCubeTransformMatrix()
{
	float scaleXandZ = uBoxPercentage * vertex_in[0].dx; // scale the unit cube in x and z dimensions 
	float scaleY = gl_in[0].gl_Position.y; // scale in the y direction by the height of the wave 
	float translateX = gl_in[0].gl_Position.x; // translation in the x direction 
	vec4 col0 = vec4(scaleXandZ, 0.0, 0.0, 0.0);
	vec4 col1 = vec4(0.0, scaleY, 0.0, 0.0);
	vec4 col2 = vec4(0.0, 0.0, scaleXandZ, 0.0);
	vec4 col3 = vec4(translateX, 0.0, 0.0, 1.0);
	return mat4(col0, col1, col2, col3);
}

CubeFace getFrontFace(mat4 t)
{
	CubeFace face;
	face.p[0] = t * CubePoints[4];
	face.p[1] = t * CubePoints[5];
	face.p[2] = t * CubePoints[7];
	face.p[3] = t * CubePoints[6];
	face.n = vec3(0.0, 0.0, 1.0);
	return face;
}

CubeFace getBackFace(mat4 t)
{
	CubeFace face;
	face.p[0] = t * CubePoints[1];
	face.p[1] = t * CubePoints[0];
	face.p[2] = t * CubePoints[2];
	face.p[3] = t * CubePoints[3];
	face.n = vec3(0.0, 0.0, - 1.0);
	return face;
}

CubeFace getTopFace(mat4 t)
{
	CubeFace face;
	face.p[0] = t * CubePoints[6];
	face.p[1] = t * CubePoints[7];
	face.p[2] = t * CubePoints[3];
	face.p[3] = t * CubePoints[2];
	face.n = vec3(0.0, 1.0, 0.0);
	return face;
}

CubeFace getBottomFace(mat4 t)
{
	CubeFace face;
	face.p[0] = t * CubePoints[0];
	face.p[1] = t * CubePoints[1];
	face.p[2] = t * CubePoints[5];
	face.p[3] = t * CubePoints[4];
	face.n = vec3(0.0, -1.0, 0.0);
	return face;
}

CubeFace getLeftFace(mat4 t)
{
	CubeFace face;
	face.p[0] = t * CubePoints[0];
	face.p[1] = t * CubePoints[2];
	face.p[2] = t * CubePoints[6];
	face.p[3] = t * CubePoints[4];
	face.n = vec3(-1.0, 0.0, 0.0);
	return face;
}

CubeFace getRightFace(mat4 t)
{
	CubeFace face;
	face.p[0] = t * CubePoints[1];
	face.p[1] = t * CubePoints[3];
	face.p[2] = t * CubePoints[7];
	face.p[3] = t * CubePoints[5];
	face.n = vec3(1.0, 0.0, 0.0);
	return face;
}

void main()
{
	mat4 modelViewMatrix = uViewMatrix * uModelMatrix;
	mat3 normalMatrix = mat3(transpose(inverse(modelViewMatrix)));
	mat4 viewProjectionMatrix = uProjectionMatrix * uViewMatrix;

	mat4 cubeTransform = getCubeTransformMatrix();
	CubeFace faces[6];
	faces[0] = getFrontFace(cubeTransform);
	faces[1] = getBackFace(cubeTransform);
	faces[2] = getTopFace(cubeTransform);
	faces[3] = getBottomFace(cubeTransform);
	faces[4] = getLeftFace(cubeTransform);
	faces[5] = getRightFace(cubeTransform);

	for(int i = 0; i < 6; i++) 
	{
		for(int j = 0; j < 4; j++) 
		{
			vec4 position = uModelMatrix * faces[i].p[j];
			vertex_out.worldPosition = position.xyz;
			vertex_out.normal = normalMatrix * faces[i].n;
			vertex_out.velocity = vertex_in[0].velocity;
			gl_Position = viewProjectionMatrix * position;		
			EmitVertex();
		}
		EndPrimitive();
	}
}
