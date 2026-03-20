#pragma once

class MouseController {
public:
    MouseController();

    // High-level input interface (isolated from GLFW details).
    void onCursorMove(double x, double y);
    void onMouseButton(bool pressed);
    void onScroll(double yoffset);

    // Programmatic controls for tests and future keyboard bindings.
    void rotateBy(float deltaYaw, float deltaPitch);
    void zoomBy(float deltaDistance);
    void reset();

    void setRotationSensitivity(float value);
    void setZoomSensitivity(float value);
    void setPitchLimits(float minPitch, float maxPitch);
    bool toggleLightByScreenClick(float windowWidth, float windowHeight, float framebufferWidth, float framebufferHeight);
    void setLightEnabled(bool enabled);

    // Query state used by rendering and assertions.
    float getYaw() const { return yaw; }
    float getPitch() const { return pitch; }
    float getDistance() const { return distance; }
    bool isLightEnabled() const { return lightEnabled; }
    bool isDragging() const { return mouseDown; }
    double getCursorX() const { return mouseX; }
    double getCursorY() const { return mouseY; }
    double getClickX() const { return clickX; }
    double getClickY() const { return clickY; }

    // Build Shadertoy iMouse payload: (x, y, clickX, clickY).
    void buildIMouse(float framebufferHeight, float outMouse[4]) const;

    static constexpr float DEFAULT_ROTATION_SENSITIVITY = 0.01f;
    static constexpr float DEFAULT_ZOOM_SENSITIVITY = 0.08f;
    static constexpr float DEFAULT_DISTANCE = 2.6f;
    static constexpr float DEFAULT_PITCH_MAX = 1.4f;
    static constexpr float DEFAULT_PITCH_MIN = -1.4f;
    static constexpr float DEFAULT_MIN_DISTANCE = 1.2f;
    static constexpr float DEFAULT_MAX_DISTANCE = 8.0f;
    static constexpr float LIGHT_SWITCH_WIDTH = 110.0f;
    static constexpr float LIGHT_SWITCH_HEIGHT = 44.0f;
    static constexpr float LIGHT_SWITCH_MARGIN_RIGHT = 24.0f;

private:
    void clampPitch();
    void clampDistance();
    bool isInsideLightSwitch(float xBottomLeft, float yBottomLeft, float framebufferWidth, float framebufferHeight) const;

    double mouseX = 0.0;
    double mouseY = 0.0;
    bool mouseDown = false;
    double clickX = 0.0;
    double clickY = 0.0;

    double dragStartX = 0.0;
    double dragStartY = 0.0;
    float dragStartYaw = 0.0f;
    float dragStartPitch = 0.0f;

    float yaw = 0.0f;
    float pitch = 0.0f;
    float distance = DEFAULT_DISTANCE;

    float rotationSensitivity = DEFAULT_ROTATION_SENSITIVITY;
    float zoomSensitivity = DEFAULT_ZOOM_SENSITIVITY;
    float minPitch = DEFAULT_PITCH_MIN;
    float maxPitch = DEFAULT_PITCH_MAX;
    float minDistance = DEFAULT_MIN_DISTANCE;
    float maxDistance = DEFAULT_MAX_DISTANCE;
    bool lightEnabled = false;
};
