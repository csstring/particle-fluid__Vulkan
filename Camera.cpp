#include "Camera.hpp"
#include <glm/gtx/transform.hpp>
#include "imgui.h"

void Camera::update(void)
{
    switch (_moveDir)
    {
    case CAMERA_MOVE_DIR::MOVE_W:
        _cameraPos += _movementSpeed * _cameraFront;
        break;
    case CAMERA_MOVE_DIR::MOVE_S:
        _cameraPos -= _movementSpeed * _cameraFront;
        break;
    case CAMERA_MOVE_DIR::MOVE_A:
        _cameraPos -= _movementSpeed * _cameraRight;
        break;
    case CAMERA_MOVE_DIR::MOVE_D:
        _cameraPos += _movementSpeed * _cameraRight;
        break;
    default:
        break;
    }
    _view = glm::lookAt(_cameraPos, _cameraPos+_cameraFront, _cameraUp);
}

void Camera::initialize(void)
{
    _yaw = -90.0;
    _pitch = 0.0f;
    _mouseSensitivity = 0.1f;
    _movementSpeed = 0.2f;
    _fov = 80.0f;
    _isFirst = true;
    _isOn = false;
    _clickOn = false;
    _cameraPos = glm::vec3(0, 0 ,-2.0);
    _cameraUp = glm::vec3(0,1,0);
    _cameraFront = glm::vec3(0,0,1);
    _worldUp = _cameraUp;
    _zFar = 50;
    _moveDir = CAMERA_MOVE_DIR::MOVE_STOP;
    projection = glm::perspective(glm::radians(_fov), 1920.f / 1080.f, _zNear, _zFar);
    updateCameraVectors(); 
}

void Camera::updateCameraVectors()
{
    glm::vec3 front;
    front.x = cos(glm::radians(_yaw)) * cos(glm::radians(_pitch));
    front.y = sin(glm::radians(_pitch));
    front.z = sin(glm::radians(_yaw)) * cos(glm::radians(_pitch));
    _cameraFront = glm::normalize(front);

    _cameraRight = glm::normalize(glm::cross(_cameraFront, _worldUp));  
    _cameraUp    = glm::normalize(glm::cross(_cameraRight, _cameraFront));
}

void Camera::ProcessMouseMovement(float xoffset, float yoffset, bool constrainPitch = true)
{
    xoffset *= _mouseSensitivity;
    yoffset *= _mouseSensitivity;

    _yaw   += xoffset;
    _pitch += yoffset;

    if (constrainPitch)
    {
        if (_pitch > 89.0f)
            _pitch = 89.0f;
        if (_pitch < -89.0f)
            _pitch = -89.0f;
    }
    updateCameraVectors();
}

void Camera::mouse_callback(double xposIn, double yposIn)
{
    Camera& _camera = Camera::getInstance();
    ImGuiIO& io = ImGui::GetIO();
    io.AddMousePosEvent(xposIn, yposIn);
    
    float xpos = static_cast<float>(xposIn);
    float ypos = static_cast<float>(yposIn);

    if (_camera._isFirst)
    {
        _camera._lastX = xpos;
        _camera._lastY = ypos;
        _camera._isFirst = false;
    }

    float xoffset = xpos - _camera._lastX;
    float yoffset = ypos - _camera._lastY;

    _camera._lastX = xpos;
    _camera._lastY = ypos;
    if (_camera._isOn == false)
        return ;
    _camera.ProcessMouseMovement(xoffset, yoffset, true);
}

glm::vec4 Camera::getWorldCursorPos(float width, float height) const
{
    const float distance = 15.0f;
    const float midlePlaneY = distance * glm::tan(glm::radians(_fov/2.0f)) * 2.0f;
    const float midlePlaneX = midlePlaneY * (width/height);
    const float ratio = height / midlePlaneY;

    const float y = -((_lastY) / ratio - midlePlaneY / 2.0f);
    const float x = _lastX / ratio - midlePlaneX / 2.0f;

    return glm::vec4(_cameraPos + _cameraFront * distance + _cameraRight * x + _cameraUp * y, 1.0f);
}