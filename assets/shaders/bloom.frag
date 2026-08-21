#version 330

in vec2 fragTexCoord;

out vec4 finalColor;

uniform sampler2D texture0;
uniform sampler2D texture1;

uniform vec3 texelSize;
uniform float bloomStrength;

const int reach = 3;

void main()
{
    vec3 scene = texture(texture0, fragTexCoord).rgb;

    vec3 spilled = vec3(0.0);
    float weight = 0.0;

    for (int down = -reach; down <= reach; ++down)
    {
        for (int across = -reach; across <= reach; ++across)
        {
            float held = exp(
                -0.35 * float((across * across) + (down * down)));

            spilled += texture(
                           texture1,
                           fragTexCoord
                               + (vec2(float(across), float(down))
                                  * texelSize.xy))
                           .rgb
                       * held;
            weight += held;
        }
    }

    finalColor = vec4(
        scene + ((spilled / weight) * bloomStrength), 1.0);
}
