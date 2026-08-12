#include <cassert> 
#include <limits>

#include "lve_camera.hpp"

namespace lve {

    void LveCamera::setOrthographicProjection(
        float left, float right, float top, float bottom, float near, float far) {
        projectionMatrix = glm::mat4{ 1.0f };
        projectionMatrix[0][0] = 2.f / (right - left);
        projectionMatrix[1][1] = 2.f / (bottom - top);
        projectionMatrix[2][2] = 1.f / (far - near);
        projectionMatrix[3][0] = -(right + left) / (right - left);
        projectionMatrix[3][1] = -(bottom + top) / (bottom - top);
        projectionMatrix[3][2] = -near / (far - near);
    }

    /*
        Why perspective projection have tan.
        Base projection matrix: 
        [2n/(r-l)        0 -(r+l)/(r-l)         0]
        [       0 2n/(b-t) -(b+t)/(b-t)         0]
        [       0        0      f/(f-n) -fn/(f-n)]
        [       0        0            1         0]

        Known fact about r, l, b, t
        Position of r = -l 
        Position of t = -b
        Vector Addition (or following the vector in order leads to) r + l = 0
        Vector Addition (or following the vector in order leads to) t + b = 0
        Because r is symmetric with l, r + |-l| = r - l = 2r = 2l
        Because t is symmetric with b, t + |-b| = t - b = 2b = 2t

        So, the matrix can be simplified to: 
        [     n/r        0            0         0]
        [       0      n/b            0         0]
        [       0        0      f/(f-n) -fn/(f-n)]
        [       0        0            1         0]

        To be able to radians, remember that tan() = opp/adj.
        If we have straight line from origin to center of near plane, this is the adjacent.
        From center of near plane to b plane, you will need half degreen (full degree means from top to bottom plane). 
        Considering this fact, we can think of: 
        b/n -> b = n * tan(theta/2)
        r/n -> r = n * tan(theta/2) * width/height 
        
        The width and height is adjustment so that fix function properly map to the frame buffer with appropriate aspect. 

        So, final perspective projection matrix is: 
        [1/(w/h * tan(theta/2)                     0            0            0]
        [                    0        1/tan(theta/2)            0            0]
        [                    0                     0      f/(f-n)    -fn/(f-n)]
        [                    0                     0            1            0]


    */
    void LveCamera::setPerspectiveProjection(float fovy, float aspect, float near, float far) {
        assert(glm::abs(aspect - std::numeric_limits<float>::epsilon()) > 0.0f);
        const float tanHalfFovy = tan(fovy / 2.f);
        projectionMatrix = glm::mat4{ 0.0f };
        projectionMatrix[0][0] = 1.f / (aspect * tanHalfFovy);
        projectionMatrix[1][1] = 1.f / (tanHalfFovy);
        projectionMatrix[2][2] = far / (far - near);
        projectionMatrix[2][3] = 1.f;
        projectionMatrix[3][2] = -(far * near) / (far - near);
    }

    void LveCamera::setViewDirection(glm::vec3 position, glm::vec3 direction, glm::vec3 up) {
        // The three liens of codes creates a orthonormal basis vector. 
        const glm::vec3 w{ glm::normalize(direction) };
        const glm::vec3 u{ glm::normalize(glm::cross(w, up)) };
        const glm::vec3 v{ glm::cross(w, u) };

        viewMatrix = glm::mat4{ 1.f };
        viewMatrix[0][0] = u.x;
        viewMatrix[1][0] = u.y;
        viewMatrix[2][0] = u.z;
        viewMatrix[0][1] = v.x;
        viewMatrix[1][1] = v.y;
        viewMatrix[2][1] = v.z;
        viewMatrix[0][2] = w.x;
        viewMatrix[1][2] = w.y;
        viewMatrix[2][2] = w.z;
        viewMatrix[3][0] = -glm::dot(u, position);
        viewMatrix[3][1] = -glm::dot(v, position);
        viewMatrix[3][2] = -glm::dot(w, position);
    }

    void LveCamera::setViewTarget(glm::vec3 position, glm::vec3 target, glm::vec3 up) {
        setViewDirection(position, target - position, up);
    }

    void LveCamera::setViewYXZ(glm::vec3 position, glm::vec3 rotation) {
        const float c3 = glm::cos(rotation.z);
        const float s3 = glm::sin(rotation.z);
        const float c2 = glm::cos(rotation.x);
        const float s2 = glm::sin(rotation.x);
        const float c1 = glm::cos(rotation.y);
        const float s1 = glm::sin(rotation.y);
        const glm::vec3 u{ (c1 * c3 + s1 * s2 * s3), (c2 * s3), (c1 * s2 * s3 - c3 * s1) };
        const glm::vec3 v{ (c3 * s1 * s2 - c1 * s3), (c2 * c3), (c1 * c3 * s2 + s1 * s3) };
        const glm::vec3 w{ (c2 * s1), (-s2), (c1 * c2) };
        viewMatrix = glm::mat4{ 1.f };
        viewMatrix[0][0] = u.x;
        viewMatrix[1][0] = u.y;
        viewMatrix[2][0] = u.z;
        viewMatrix[0][1] = v.x;
        viewMatrix[1][1] = v.y;
        viewMatrix[2][1] = v.z;
        viewMatrix[0][2] = w.x;
        viewMatrix[1][2] = w.y;
        viewMatrix[2][2] = w.z;
        viewMatrix[3][0] = -glm::dot(u, position);
        viewMatrix[3][1] = -glm::dot(v, position);
        viewMatrix[3][2] = -glm::dot(w, position);
    }

}  // namespace lve