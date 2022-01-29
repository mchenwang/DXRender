#include "Camera.h"

Camera::Camera(
    float windowWidth, float windowHeight,
    float fov, float near, float far,
    XMFLOAT4 position, XMFLOAT4 focusPoint, XMFLOAT4 upDirection)
    : m_fov(fov)
    , m_near(near)
    , m_far(far)
    , m_position(position)
    , m_focusPoint(focusPoint)
    , m_upDirection(upDirection)
{
    SetAspectRatio(windowWidth, windowHeight);
    SetViewMatrix();
    SetProjectionMatrix();
}

void Camera::SetAspectRatio(float windowWidth, float windowHeight)
{
    m_aspectRatio = windowWidth / windowHeight;
}

void Camera::SetViewMatrix()
{
    const XMVECTOR position = XMLoadFloat4(&m_position);
    const XMVECTOR focusPoint = XMLoadFloat4(&m_focusPoint);
    const XMVECTOR upDirection = XMLoadFloat4(&m_upDirection);

    m_viewMatrix = XMMatrixLookAtLH(position, focusPoint, upDirection);
}

void Camera::SetProjectionMatrix()
{
    m_projectionMatrix = XMMatrixPerspectiveFovLH(
        XMConvertToRadians(m_fov),
        m_aspectRatio, m_near, m_far);
}

void Camera::UpdateAspectRatio(float aspectRatio)
{
    m_aspectRatio = aspectRatio;
}
void Camera::UpdateAspectRatio(float windowWidth, float windowHeight)
{
    SetAspectRatio(windowWidth, windowHeight);
}
void Camera::UpdateFoV(float FoV)
{
    m_fov = FoV;
}
void Camera::UpdatePosition(XMFLOAT4 position)
{
    m_position = position;
}
void Camera::UpdateFocusPoint(XMFLOAT4 focusPoint)
{
    m_focusPoint = focusPoint;
}
void Camera::UpdateUpDirection(XMFLOAT4 upDirection)
{
    m_upDirection = upDirection;
}

XMMATRIX Camera::GetViewMatrix()
{
    SetViewMatrix();
    return m_viewMatrix;
}

XMMATRIX Camera::GetProjectionMatrix()
{
    SetProjectionMatrix();
    return m_projectionMatrix;
}