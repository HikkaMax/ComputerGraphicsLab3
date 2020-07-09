#version 330 core
layout(location = 0) in vec4 vertex;
layout(location = 1) in vec4 normal;
layout(location = 2) in vec2 texCoords;
layout(location = 4) in vec3 tangent;
layout(location = 5) in vec3 bitangent;


out vec2 vTexCoords;
out vec3 vFragPosition;
out vec3 vNormal;
out vec3 skyBoxTex;
out mat3 TBN;


uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform int verType;
uniform vec3 offsets[100];

void main()
{
  if (verType == 0)
  {
      skyBoxTex = vec3(vertex[0], vertex[1], vertex[2]);
      vec4 pos = projection * view * vertex;
      gl_Position = pos.xyww;
  }
  else if (verType == 1)
  {
      vec3 offset = offsets[gl_InstanceID];

      gl_Position = projection * view * model * (vertex + vec4(offset, 0.0f));

      vTexCoords = texCoords;
      vFragPosition = vec3(model * vertex) + offset * gl_InstanceID / 100;
      vNormal = mat3(transpose(inverse(model))) * normal.xyz;
  }
  else if (verType == 2)
  {
      gl_Position = projection * view * model * vertex;

      vTexCoords = texCoords;
      vFragPosition = vec3(model * vertex);
      vNormal = normalize(mat3(transpose(inverse(model))) * normal.xyz);

      mat3 normalMatrix = transpose(inverse(mat3(model)));
      vec3 T = normalize(normalMatrix * tangent);
      vec3 N = normalize(normalMatrix * vNormal);
      T = normalize(T - dot(T, N) * N);
      vec3 B = cross(N, T);
    
      TBN = transpose(mat3(T, B, N));    
  }
  else
  {
      gl_Position = projection * view * model * vertex;

      vTexCoords = texCoords;
      vFragPosition = vec3(model * vertex);
      vNormal = mat3(transpose(inverse(model))) * normal.xyz;
  }
}