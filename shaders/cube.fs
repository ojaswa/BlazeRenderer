/***************************************************************************
**                                                                        **
**  BlazeRenderer - An OpenGL based real-time volume renderer             **
**  Copyright (C) 2016-2018 Graphics Research Group, IIIT Delhi           **
**                                                                        **
**  This program is free software: you can redistribute it and/or modify  **
**  it under the terms of the GNU General Public License as published by  **
**  the Free Software Foundation, either version 3 of the License, or     **
**  (at your option) any later version.                                   **
**                                                                        **
**  This program is distributed in the hope that it will be useful,       **
**  but WITHOUT ANY WARRANTY; without even the implied warranty of        **
**  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         **
**  GNU General Public License for more details.                          **
**                                                                        **
**  You should have received a copy of the GNU General Public License     **
**  along with this program.  If not, see http://www.gnu.org/licenses/.   **
**                                                                        **
****************************************************************************
**           Author: Ojaswa Sharma                                        **
**           E-mail: ojaswa@iiitd.ac.in                                   **
**           Date  : 14.12.2016                                           **
****************************************************************************/

#version 410

in vec3 vPosition;
out vec4 fColor;

uniform sampler3D uTexVol; // Volumetric texture
uniform sampler1D uTexTF1D; // 256 length RGBA TF texture
uniform sampler2D uTexNoise; //32x32 luminance noise texture
uniform sampler3D uTexVolNormals; // Volumetric texture normals

uniform float uTime;
uniform mat4 uView;
uniform float uStepSize;
uniform int uInterpolationType;
uniform int uUseJittering;
uniform vec3 uBBox;
uniform int uPerformPhongShading;

#define SHININESS 128

// Return exit intersection point of the given ray with cuboid centered at origin
// p = eye + t*direction, t \in [t_begin, t_end]
// This routine will return t_end
float cuboid_intersect(vec3 eye, vec3 direction)
{
    //Liang-Barsky clipping algorithm.
    //Need to look only at exit points
    float dimX = uBBox.x/2.0; // L = -dimX, R = dimX
    float dimY = uBBox.y/2.0; // B = -dimY, T = dimY
    float dimZ = uBBox.z/2.0; // N = -dimZ, F = dimZ

    float Cx, Cy, Cz, qx, qy, qz;// parameters of exit points
    if(direction.x > 0) { //+X plane is exit
        Cx = direction.x;
        qx = dimX - eye.x;
    } else { //-X plane is exit
        Cx = -direction.x;
        qx = dimX + eye.x;
    }

    if(direction.y > 0) { //+Y plane is exit
        Cy = direction.y;
        qy = dimY - eye.y;
    } else { //-Y plane is exit
        Cy = -direction.y;
        qy = dimY + eye.y;
    }

    if(direction.z > 0) { //+Z plane is exit
        Cz = direction.z;
        qz = dimZ - eye.z;
    } else { //-Z plane is exit
        Cz = -direction.z;
        qz = dimZ + eye.z;
    }

    float tx = qx/Cx;
    float ty = qy/Cy;
    float tz = qz/Cz;

    return min(tx, min(ty, tz));
}

vec3 vert2tex(vec3 v)
{
    return (v/uBBox + vec3(0.5, 0.5, 0.5));
}

vec4 shade(vec3 fPos, vec4 fColor, vec3 dir, vec3 normal, vec3 lightPos) {
    vec3 lightVec = normalize(lightPos - fPos);
    vec3 diffuse = fColor.rgb * clamp(abs(dot(normal, lightVec)), 0, 1);//Two-sided lighting
    vec3 reflected = reflect(-lightVec, normal);
    vec3 specular = vec3(clamp(pow(max(dot(dir, reflected), 0), SHININESS), 0, 1));
    return vec4(diffuse + specular, fColor.a);
}

void main(void) {
    mat4 invView = inverse(uView);
    vec3 eye = vec3(invView[3][0], invView[3][1],invView[3][2]); //Last column

    //Begin raycasting into the scene
    vec3 fPosition = vPosition;
    vec3 dir = normalize((fPosition - eye));
    float delta_t = cuboid_intersect(eye, dir) - length(fPosition - eye); // t_end - t_begin

    if(uUseJittering == 1)
        fPosition += 0.002*texture(uTexNoise, gl_FragCoord.xy/vec2(32, 32)).x; //Jitter the position
    vec3 delta_dir = dir * uStepSize; // normalize and pre-multiply by stepsize for efficiency

    //Raycasting - front to back compositing
    vec4 color = vec4(0, 0, 0, 0); //alpha is computed within it.
    float texVol_sample;
    vec4 texRGBA_sample;
    vec3 normal;
    float normal_mag;
    vec3 lightPos = eye; //Headlight

    for(float s = 0; s < delta_t; s += uStepSize) { //Front to back
        texVol_sample = texture(uTexVol, vert2tex(fPosition)).r;
        texRGBA_sample = texture(uTexTF1D, texVol_sample); //RGBA Sample
        if(uPerformPhongShading == 1) {
            normal = texture(uTexVolNormals, vert2tex(fPosition)).rgb* 2.0 - 1.0;
            normal_mag = length(normal);
            if (normal_mag > 0.1 && s > uStepSize) {
                texRGBA_sample = shade(fPosition, texRGBA_sample, -dir, normalize(normal), lightPos);
                //texRGBA_sample.rgb  *= normal_mag;
            }
        }
        if(texRGBA_sample.a > 0.0) {
            color += (1.0 - color.a)*texRGBA_sample;
        }
        if(color.a > 0.95) break; //Early ray termination
        fPosition += delta_dir;
    }
    fColor = vec4(color);
}
