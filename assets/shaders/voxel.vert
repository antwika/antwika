#version 330

in vec3 vertexPosition;
in vec3 vertexNormal;
in vec2 vertexTexCoord;
in vec4 vertexColor;

out vec3 fragPosition;
out vec3 fragNormal;
out vec2 fragTexCoord;
out vec4 fragColor;
out vec3 fragLocal;
out vec3 fragLocalNormal;

uniform mat4 mvp;
uniform mat4 matModel;
uniform mat4 matNormal;

void main()
{
    vec4 stands = matModel * vec4(vertexPosition, 1.0);

    fragPosition = stands.xyz;
    fragNormal = vec3(matNormal * vec4(vertexNormal, 0.0));
    fragTexCoord = vertexTexCoord;
    fragColor = vertexColor;
    fragLocal = vertexPosition;
    fragLocalNormal = vertexNormal;

    gl_Position = mvp * vec4(vertexPosition, 1.0);
}
