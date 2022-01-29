#ifndef __CAMERA_H__
#define __CAMERA_H__

#include <DirectXMath.h>

using namespace DirectX;

class Camera
{
private:
    XMFLOAT4 m_position;
    XMFLOAT4 m_focusPoint;
    XMFLOAT4 m_upDirection;
    // view frustum data
    float m_fov;
    float m_near;
    float m_far;
    float m_aspectRatio;

    XMMATRIX m_viewMatrix;
    XMMATRIX m_projectionMatrix;

    void SetAspectRatio(float windowWidth, float windowHeight);
    void SetViewMatrix();
    void SetProjectionMatrix();
public:
    Camera(
        float windowWidth,
        float windowHeight,
        float fov = 45.f,
        float near = 0.001f,
        float far = 100.f,
        XMFLOAT4 position = XMFLOAT4(0.f, 0.f, 5.f, 1.f),
        XMFLOAT4 focusPoint = XMFLOAT4(0.f, 0.f, 0.f, 1.f),
        XMFLOAT4 upDirection = XMFLOAT4(0.f, 1.f, 0.f, 0.f)
    );
    ~Camera() = default;

    void UpdateAspectRatio(float aspectRatio);
    void UpdateAspectRatio(float windowWidth, float windowHeight);
    void UpdateFoV(float FoV);
    void UpdatePosition(XMFLOAT4 position);
    void UpdateFocusPoint(XMFLOAT4 focusPoint);
    void UpdateUpDirection(XMFLOAT4 upDirection);

    XMMATRIX GetViewMatrix();
    XMMATRIX GetProjectionMatrix();
};


#endif