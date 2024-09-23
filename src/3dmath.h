#ifndef D3DMATH
#define D3DMATH

#define PI 3.14159265358979323846
#include <math.h>

template <typename T>
struct vec2_base {
    T x, y;
    vec2_base(T x, T y) : x(x), y(y) {}
    vec2_base() : x(0), y(0) {}
    T* value_ptr() {
        return &x;
    }

    vec2_base operator+(const vec2_base& other) const {
        return vec2_base(x + other.x, y + other.y);
    }

    vec2_base operator+(const T other) const {
        return vec2_base(x + other, y + other);
    }

    vec2_base& operator+=(const vec2_base& other){
        x += other.x;
        y += other.y;
        return *this;
    }

    vec2_base& operator+=(const T other){
        x += other;
        y += other;
        return *this;
    }

    vec2_base operator*(const T other) const {
        return vec2_base(x * other, y * other);
    }

    vec2_base& operator*=(const vec2_base& other){
        x *= other.x;
        y *= other.y;
        return *this;
    }

    vec2_base& operator*=(const T other){
        x *= other;
        y *= other;
        return *this;
    }


    vec2_base operator-(const vec2_base& other) const {
        return vec2_base(x - other.x, y - other.y);
    }

    vec2_base operator-(const T other) const {
        return vec2_base(x - other, y - other);
    }

    vec2_base& operator-=(const vec2_base& other){
        x -= other.x;
        y -= other.y;
        return *this;
    }

    vec2_base& operator-=(const T other){
        x -= other;
        y -= other;
        return *this;
    }

    vec2_base operator/(const T other) const {
        return vec2_base(x / other, y / other);
    }

    vec2_base& operator/=(const T other){
        x /= other;
        y /= other;
        return *this;
    }

    bool operator==(const vec2_base& other) const {
        return x == other.x && y == other.y;
    }

    T mag() const {
        return sqrt(x*x + y*y);
    }

    vec2_base normalize(){
        float magVal = mag();
        if (magVal != 0){
            x /= magVal;
            y /= magVal;
        }
        return *this;
    }
};

typedef vec2_base<float> vec2;
typedef vec2_base<int> Ivec2;

template <typename T>
struct vec3_base {
    T x, y, z;
    vec3_base(T x, T y, T z) : x(x), y(y), z(z) {}
    vec3_base() : x(0), y(0), z(0) {}

    vec3_base operator+(const vec3_base& other) const {
        return vec3_base(x + other.x, y + other.y, z + other.z);
    }

    vec3_base operator+(const T other) const {
        return vec3_base(x + other, y + other, z + other);
    }
    
    vec3_base& operator+=(const vec3_base& other){
        x += other.x;
        y += other.y;
        z += other.z;
        return *this;
    }

    vec3_base& operator+=(const T other){
        x += other;
        y += other;
        z += other;
        return *this;
    }

    vec3_base operator-(const vec3_base& other) const {
        return vec3_base(x - other.x, y - other.y, z - other.z);
    }

    vec3_base operator-(const T other) const {
        return vec3_base(x - other, y - other, z - other);
    }

    vec3_base& operator-=(const vec3_base& other){
        x -= other.x;
        y -= other.y;
        z -= other.z;
        return *this;
    }

    vec3_base& operator-=(const T other){
        x -= other;
        y -= other;
        z -= other;
        return *this;
    }

    vec3_base operator*(const T other) const {
        return vec3_base(x * other, y * other, z * other);
    }

    vec3_base& operator*=(const T other){
        x *= other;
        y *= other;
        z *= other;
        return *this;
    }
    
    vec3_base operator/(const T other) const {
        return vec3_base(x / other, y / other, z / other);
    }

    vec3_base& operator/=(const T other){
        x /= other;
        y /= other;
        z /= other;
        return *this;
    }

    bool operator==(const vec3_base& other){
        return (x==other.x && y==other.y && z==other.z);
    }

    bool operator!=(const vec3_base& other){
        return !(x==other.x && y==other.y && z==other.z);
    }

    T mag() const {
        return sqrt(x*x + y*y + z*z);
    }

    vec3_base normalize(){
        float magVal = mag();
        if (magVal != 0){
            x /= magVal;
            y /= magVal;
            z /= magVal;
        }
        return *this;
    }

};

typedef vec3_base<float> vec3;

template <typename T>
struct vec4_base {
    T x, y, z, w;
    vec4_base(T x, T y, T z, T w) : x(x), y(y), z(z), w(w) {}
    vec4_base() : x(0), y(0), z(0), w(0) {}

    bool operator==(const vec4_base &other) const{
        return (x == other.x && y == other.y && z == other.z && w == other.w);
    }

    bool operator!=(const vec4_base &other) const{
        return !(x == other.x && y == other.y && z == other.z && w == other.w);
    }
};

typedef vec4_base<float> vec4;

struct mat4 {

public:
    float 
        x11, x21, x31, x41,    //  x11 | x12 | x13 | x14
        x12, x22, x32, x42,    //  x21 | x22 | x23 | x24
        x13, x23, x33, x43,    //  x31 | x32 | x33 | x34
        x14, x24, x34, x44;    //  x41 | x42 | x43 | x44
public:

    mat4(float x11, float x12, float x13, float x14,
        float x21, float x22, float x23, float x24,
        float x31, float x32, float x33, float x34,
        float x41, float x42, float x43, float x44)
        :
        x11(x11), x12(x12), x13(x13), x14(x14),
        x21(x21), x22(x22), x23(x23), x24(x24),
        x31(x31), x32(x32), x33(x33), x34(x34),
        x41(x41), x42(x42), x43(x43), x44(x44)
    {}

    mat4() : 
        x11(0), x12(0), x13(0), x14(0),
        x21(0), x22(0), x23(0), x24(0),
        x31(0), x32(0), x33(0), x34(0),
        x41(0), x42(0), x43(0), x44(0)
    {}

    void translate(vec3 translation) {
        x14 += translation.x;
        x24 += translation.y;
        x34 += translation.z;
    }

    float* value_ptr() {
        return &x11;
    }

    static mat4 identity() {

        return mat4(
            1, 0, 0, 0,
            0, 1, 0, 0,
            0, 0, 1, 0,
            0, 0, 0, 1
        );
    }

    static mat4 ortho(float left, float right, float top, float bottom, float nearr, float farr) {
        return mat4(
            2 / (right - left), 0                 , 0                  , -(right + left) / (right - left),
            0                 , 2 / (top - bottom), 0                  , -(top + bottom) / (top - bottom),
            0                 , 0                 , -2 / (farr - nearr), -(farr + nearr) / (farr - nearr),
            0                 , 0                 , 0                  , 1
        );
        
    }

    static mat4 Perspective(float fov, float aspect, float zNear, float zFar){
        float S = 1.0f / tanf(fov*PI/180/2);
        mat4 mat = {
            S/aspect, 0, 0, 0,
                   0, S, 0, 0,
                   0, 0, -zFar / (zFar - zNear), -zFar * zNear / (zFar - zNear),
                   0, 0, -1, 0,
        };
        return mat;
    }

    

    static mat4 ExtendedPerspective(float fovx, float fovy, float aspect, float zNear, float zFar){
        float Sx = 1.0f / tanf(fovx*PI/180/2);
        float Sy = 1.0f / tanf(fovy*PI/180/2);
        mat4 mat = {
            Sx/aspect, 0, 0, 0,
            0,        Sy, 0, 0,
            0,         0, -zFar / (zFar - zNear), -zFar * zNear / (zFar - zNear),
            0,         0, -1, 0
        };
        return mat;
    }

    static mat4 ortho2D(float left, float right, float top, float bottom) {
        return ortho(left, right, top, bottom, -1, 1);
    }

    mat4 transpose(){
        return {
            x11, x12, x13, x14,    //  x11 | x12 | x13 | x14
            x21, x22, x23, x24,    //  x21 | x22 | x23 | x24
            x31, x32, x33, x34,    //  x31 | x32 | x33 | x34
            x41, x42, x43, x44     //  x41 | x42 | x43 | x44
        };
    }

    static mat4 mul(mat4 it, mat4 other) {
        mat4 mat = {
            // First row
            it.x11 * other.x11 + it.x12 * other.x21 + it.x13 * other.x31 + it.x14 * other.x41, // m11
            it.x11 * other.x12 + it.x12 * other.x22 + it.x13 * other.x32 + it.x14 * other.x42, // m12
            it.x11 * other.x13 + it.x12 * other.x23 + it.x13 * other.x33 + it.x14 * other.x43, // m13
            it.x11 * other.x14 + it.x12 * other.x24 + it.x13 * other.x34 + it.x14 * other.x44, // m14

            // Second row
            it.x21 * other.x11 + it.x22 * other.x21 + it.x23 * other.x31 + it.x24 * other.x41, // m21
            it.x21 * other.x12 + it.x22 * other.x22 + it.x23 * other.x32 + it.x24 * other.x42, // m22
            it.x21 * other.x13 + it.x22 * other.x23 + it.x23 * other.x33 + it.x24 * other.x43, // m23
            it.x21 * other.x14 + it.x22 * other.x24 + it.x23 * other.x34 + it.x24 * other.x44, // m24

            // Third row
            it.x31 * other.x11 + it.x32 * other.x21 + it.x33 * other.x31 + it.x34 * other.x41, // m31
            it.x31 * other.x12 + it.x32 * other.x22 + it.x33 * other.x32 + it.x34 * other.x42, // m32
            it.x31 * other.x13 + it.x32 * other.x23 + it.x33 * other.x33 + it.x34 * other.x43, // m33
            it.x31 * other.x14 + it.x32 * other.x24 + it.x33 * other.x34 + it.x34 * other.x44, // m34

            // Fourth row
            it.x41 * other.x11 + it.x42 * other.x21 + it.x43 * other.x31 + it.x44 * other.x41, // m41
            it.x41 * other.x12 + it.x42 * other.x22 + it.x43 * other.x32 + it.x44 * other.x42, // m42
            it.x41 * other.x13 + it.x42 * other.x23 + it.x43 * other.x33 + it.x44 * other.x43, // m43
            it.x41 * other.x14 + it.x42 * other.x24 + it.x43 * other.x34 + it.x44 * other.x44  // m44
        };

        return mat;
    }
};

struct quat {
    float w, x, y, z;

     static quat fromEuler(float pitch, float yaw, float roll) {
        // Half angles
        float cy = cos(pitch * 0.5f);   // Yaw
        float sy = sin(pitch * 0.5f);
        float cp = cos(yaw * 0.5f); // Pitch
        float sp = sin(yaw * 0.5f);
        float cr = cos(roll * 0.5f);  // Roll
        float sr = sin(roll * 0.5f);

        quat q;
        q.w = cr * cp * cy + sr * sp * sy;
        q.x = sr * cp * cy - cr * sp * sy;
        q.y = cr * sp * cy + sr * cp * sy;
        q.z = cr * cp * sy - sr * sp * cy;

        return q;
    }

    // Convert quaternion to a 4x4 rotation matrix
    mat4 toMatrix() const {
        float xx = x * x;
        float yy = y * y;
        float zz = z * z;
        float xy = x * y;
        float xz = x * z;
        float yz = y * z;
        float wx = w * x;
        float wy = w * y;
        float wz = w * z;

        return {
            1 - 2 * (yy + zz),  2 * (xy - wz),      2 * (xz + wy),      0,
            2 * (xy + wz),      1 - 2 * (xx + zz),  2 * (yz - wx),      0,
            2 * (xz - wy),      2 * (yz + wx),      1 - 2 * (xx + yy),  0,
            0,                  0,                  0,                  1
        };
    }
};

// Function to convert degrees to radians
float degrees_to_radians(float degrees);

// Function to convert radians to degrees
float radians_to_degrees(float radians);

#endif