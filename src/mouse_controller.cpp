#include "mouse_controller.h"
#include <algorithm>

MouseController::MouseController()
    : mouseX(0.0), mouseY(0.0), mouseDown(false),
      clickX(0.0), clickY(0.0),
      dragStartX(0.0), dragStartY(0.0),
      dragStartYaw(0.0f), dragStartPitch(0.0f),
      yaw(0.0f), pitch(0.0f), distance(DEFAULT_DISTANCE),
      rotationSensitivity(DEFAULT_ROTATION_SENSITIVITY),
      zoomSensitivity(DEFAULT_ZOOM_SENSITIVITY),
      minPitch(DEFAULT_PITCH_MIN), maxPitch(DEFAULT_PITCH_MAX),
    minDistance(DEFAULT_MIN_DISTANCE), maxDistance(DEFAULT_MAX_DISTANCE),
    lightEnabled(false) {
}

void MouseController::onCursorMove(double x, double y) {
    mouseX = x;
    mouseY = y;

    if (mouseDown) {
        double dy = y - dragStartY;
        double dx = x - dragStartX;
        yaw = dragStartYaw - static_cast<float>(dx) * rotationSensitivity;
        pitch = dragStartPitch + static_cast<float>(dy) * rotationSensitivity;
        clampPitch();
    }
}

void MouseController::onMouseButton(bool pressed) {
    mouseDown = pressed;
    if (pressed) {
        clickX = mouseX;
        clickY = mouseY;
        dragStartX = mouseX;
        dragStartY = mouseY;
        dragStartYaw = yaw;
        dragStartPitch = pitch;
    }
}

void MouseController::onScroll(double yoffset) {
    zoomBy(-static_cast<float>(yoffset) * zoomSensitivity);
}

void MouseController::rotateBy(float deltaYaw, float deltaPitch) {
    yaw += deltaYaw;
    pitch += deltaPitch;
    clampPitch();
}

void MouseController::zoomBy(float deltaDistance) {
    distance += deltaDistance;
    clampDistance();
}

void MouseController::reset() {
    yaw = 0.0f;
    pitch = 0.0f;
    distance = DEFAULT_DISTANCE;
}

void MouseController::setRotationSensitivity(float value) {
    if (value > 0.0f) {
        rotationSensitivity = value;
    }
}

void MouseController::setZoomSensitivity(float value) {
    if (value > 0.0f) {
        zoomSensitivity = value;
    }
}

void MouseController::setPitchLimits(float minPitchValue, float maxPitchValue) {
    if (minPitchValue > maxPitchValue) {
        return;
    }
    minPitch = minPitchValue;
    maxPitch = maxPitchValue;
    clampPitch();
}

bool MouseController::toggleLightByScreenClick(float windowWidth, float windowHeight, float framebufferWidth, float framebufferHeight) {
    if (windowWidth <= 0.0f || windowHeight <= 0.0f || framebufferWidth <= 0.0f || framebufferHeight <= 0.0f) {
        return false;
    }

    // GLFW cursor is in window coordinates; convert to framebuffer coordinates for shader-space hit test.
    float sx = framebufferWidth / windowWidth;
    float sy = framebufferHeight / windowHeight;
    float xBottomLeft = static_cast<float>(mouseX) * sx;
    float yBottomLeft = framebufferHeight - static_cast<float>(mouseY) * sy;

    if (!isInsideLightSwitch(xBottomLeft, yBottomLeft, framebufferWidth, framebufferHeight)) {
        return false;
    }

    lightEnabled = !lightEnabled;
    return true;
}

void MouseController::setLightEnabled(bool enabled) {
    lightEnabled = enabled;
}

void MouseController::buildIMouse(float framebufferHeight, float outMouse[4]) const {
    float my = framebufferHeight - static_cast<float>(mouseY);
    float cmy = framebufferHeight - static_cast<float>(clickY);

    outMouse[0] = static_cast<float>(mouseX);
    outMouse[1] = my;
    outMouse[2] = mouseDown ? static_cast<float>(clickX) : 0.0f;
    outMouse[3] = mouseDown ? cmy : 0.0f;
}

void MouseController::clampPitch() {
    pitch = std::max(minPitch, std::min(maxPitch, pitch));
}

void MouseController::clampDistance() {
    distance = std::max(minDistance, std::min(maxDistance, distance));
}

bool MouseController::isInsideLightSwitch(float xBottomLeft, float yBottomLeft, float framebufferWidth, float framebufferHeight) const {
    float minX = framebufferWidth - LIGHT_SWITCH_WIDTH - LIGHT_SWITCH_MARGIN_RIGHT;
    float minY = 0.5f * framebufferHeight - 0.5f * LIGHT_SWITCH_HEIGHT;
    float maxX = minX + LIGHT_SWITCH_WIDTH;
    float maxY = minY + LIGHT_SWITCH_HEIGHT;

    return xBottomLeft >= minX && xBottomLeft <= maxX && yBottomLeft >= minY && yBottomLeft <= maxY;
}
