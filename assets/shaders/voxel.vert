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
out vec3 fragJitter;

uniform mat4 mvp;
uniform mat4 matModel;
uniform mat4 matNormal;

uniform float jitterAmount;
uniform float spriteLit;

// Every corner wobbles by a hash of where it stands on the grid, so
// the faces meeting at a corner move as one and no seams open. Only
// the drawn point moves: the varyings keep the flat-face values, so
// lighting, shadows and hiding read the world as if nothing bent.
vec3 jitterOf(vec3 corner)
{
    uvec3 cell = uvec3(ivec3(floor((corner * 16.0) + vec3(0.5))));
    uint seed = (cell.x * 0x9E3779B9u)
                ^ (cell.y * 0x85EBCA6Bu)
                ^ (cell.z * 0xC2B2AE35u);
    uvec3 mixed =
        (uvec3(seed) + uvec3(0x27D4EB2Fu, 0x165667B1u, 0x94D049BBu))
        * 0x9E3779B9u;

    mixed = (mixed ^ (mixed >> 15u)) * 0x85EBCA6Bu;
    mixed = mixed ^ (mixed >> 13u);

    // Points off the half-voxel grid belong to stair steps, whose
    // third-height treads a full wobble would mangle, and to the
    // sunken bevel bands along open edges, which a full wobble
    // would crumple; both sway less. Keying on the position keeps
    // faces that share a corner in agreement, and a flight's
    // boundary ring sits on the grid, staying sealed to its flat
    // neighbours.
    float sway =
        ((cell.x | cell.y | cell.z) & 7u) != 0u ? 0.35 : 1.0;

    return ((vec3(mixed & uvec3(0xFFFFu)) / 32767.5) - vec3(1.0))
           * (jitterAmount * sway);
}

void main()
{
    vec4 stands = matModel * vec4(vertexPosition, 1.0);

    fragPosition = stands.xyz;
    fragNormal = vec3(matNormal * vec4(vertexNormal, 0.0));
    fragTexCoord = vertexTexCoord;
    fragColor = vertexColor;
    fragLocal = vertexPosition;
    fragLocalNormal = vertexNormal;
    fragJitter = spriteLit > 0.5 ? vec3(0.0) : jitterOf(vertexPosition);

    gl_Position = mvp * vec4(vertexPosition + fragJitter, 1.0);
}
