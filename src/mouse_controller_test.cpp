#include <cmath>
#include <iostream>
#include <string>

#include "mouse_controller.h"

struct TestResult {
    std::string name;
    bool passed;
    std::string message;
};

static void printTestResult(const TestResult& result) {
    const char* status = result.passed ? "PASS" : "FAIL";
    std::cout << status << ": " << result.name;
    if (!result.message.empty()) {
        std::cout << " (" << result.message << ")";
    }
    std::cout << std::endl;
}

#define ASSERT_NEAR(actual, expected, tolerance) \
    if (std::fabs((actual) - (expected)) > (tolerance)) { \
        return TestResult{__func__, false, "expected " + std::to_string(expected) + " but got " + std::to_string(actual)}; \
    }

#define ASSERT_TRUE(condition) \
    if (!(condition)) { \
        return TestResult{__func__, false, "condition is false"}; \
    }

#define ASSERT_FALSE(condition) \
    if (condition) { \
        return TestResult{__func__, false, "condition is true"}; \
    }

#define TEST_PASS return TestResult{__func__, true, ""}

static TestResult test_default_state() {
    MouseController controller;
    ASSERT_NEAR(controller.getYaw(), 0.0f, 0.001f);
    ASSERT_NEAR(controller.getPitch(), 0.0f, 0.001f);
    ASSERT_NEAR(controller.getDistance(), MouseController::DEFAULT_DISTANCE, 0.001f);
    ASSERT_FALSE(controller.isDragging());
    TEST_PASS;
}

static TestResult test_drag_updates_rotation() {
    MouseController controller;
    controller.onCursorMove(100.0, 200.0);
    controller.onMouseButton(true);
    controller.onCursorMove(150.0, 250.0);

    ASSERT_NEAR(controller.getYaw(), -0.5f, 0.001f);
    ASSERT_NEAR(controller.getPitch(), 0.5f, 0.001f);
    ASSERT_TRUE(controller.isDragging());
    TEST_PASS;
}

static TestResult test_release_stops_dragging() {
    MouseController controller;
    controller.onCursorMove(20.0, 30.0);
    controller.onMouseButton(true);
    controller.onMouseButton(false);

    ASSERT_FALSE(controller.isDragging());
    TEST_PASS;
}

static TestResult test_rotation_api_rotate_by() {
    MouseController controller;
    controller.rotateBy(0.25f, 0.35f);

    ASSERT_NEAR(controller.getYaw(), 0.25f, 0.001f);
    ASSERT_NEAR(controller.getPitch(), 0.35f, 0.001f);
    TEST_PASS;
}

static TestResult test_zoom_api_scroll_and_zoomby() {
    MouseController controller;
    controller.onScroll(1.0); // zoom in -> smaller distance
    float afterScroll = controller.getDistance();
    controller.zoomBy(0.5f); // zoom out

    ASSERT_TRUE(afterScroll < MouseController::DEFAULT_DISTANCE);
    ASSERT_NEAR(controller.getDistance(), afterScroll + 0.5f, 0.001f);
    TEST_PASS;
}

static TestResult test_pitch_clamp_with_default_limits() {
    MouseController controller;
    controller.rotateBy(0.0f, 100.0f);
    ASSERT_NEAR(controller.getPitch(), MouseController::DEFAULT_PITCH_MAX, 0.001f);

    controller.rotateBy(0.0f, -100.0f);
    ASSERT_NEAR(controller.getPitch(), MouseController::DEFAULT_PITCH_MIN, 0.001f);
    TEST_PASS;
}

static TestResult test_custom_pitch_limits() {
    MouseController controller;
    controller.setPitchLimits(-0.5f, 0.5f);
    controller.rotateBy(0.0f, 1.0f);
    ASSERT_NEAR(controller.getPitch(), 0.5f, 0.001f);

    controller.rotateBy(0.0f, -2.0f);
    ASSERT_NEAR(controller.getPitch(), -0.5f, 0.001f);
    TEST_PASS;
}

static TestResult test_custom_sensitivity() {
    MouseController controller;
    controller.setRotationSensitivity(0.02f);
    controller.onCursorMove(100.0, 200.0);
    controller.onMouseButton(true);
    controller.onCursorMove(150.0, 200.0);

    ASSERT_NEAR(controller.getYaw(), -1.0f, 0.001f);
    TEST_PASS;
}

static TestResult test_reset_restores_view_state() {
    MouseController controller;
    controller.rotateBy(0.5f, 0.4f);
    controller.zoomBy(2.0f);
    controller.reset();

    ASSERT_NEAR(controller.getYaw(), 0.0f, 0.001f);
    ASSERT_NEAR(controller.getPitch(), 0.0f, 0.001f);
    ASSERT_NEAR(controller.getDistance(), MouseController::DEFAULT_DISTANCE, 0.001f);
    TEST_PASS;
}

static TestResult test_build_imouse_payload() {
    MouseController controller;
    controller.onCursorMove(120.0, 25.0);
    controller.onMouseButton(true);
    controller.onCursorMove(140.0, 35.0);

    float iMouse[4] = {0.0f, 0.0f, 0.0f, 0.0f};
    controller.buildIMouse(200.0f, iMouse);

    ASSERT_NEAR(iMouse[0], 140.0f, 0.001f);
    ASSERT_NEAR(iMouse[1], 165.0f, 0.001f);
    ASSERT_NEAR(iMouse[2], 120.0f, 0.001f);
    ASSERT_NEAR(iMouse[3], 175.0f, 0.001f);

    controller.onMouseButton(false);
    controller.buildIMouse(200.0f, iMouse);
    ASSERT_NEAR(iMouse[2], 0.0f, 0.001f);
    ASSERT_NEAR(iMouse[3], 0.0f, 0.001f);
    TEST_PASS;
}

static TestResult test_light_toggle_switch_hit() {
    MouseController controller;
    const float width = 800.0f;
    const float height = 600.0f;

    // Click inside switch rect: x in [666, 776], y(bottom-left) in [278, 322].
    controller.onCursorMove(700.0, 300.0);
    bool toggled = controller.toggleLightByScreenClick(width, height, width, height);
    ASSERT_TRUE(toggled);
    ASSERT_TRUE(controller.isLightEnabled());

    toggled = controller.toggleLightByScreenClick(width, height, width, height);
    ASSERT_TRUE(toggled);
    ASSERT_FALSE(controller.isLightEnabled());
    TEST_PASS;
}

static TestResult test_light_toggle_switch_miss() {
    MouseController controller;
    const float width = 800.0f;
    const float height = 600.0f;

    controller.setLightEnabled(false);
    controller.onCursorMove(100.0, 100.0);
    bool toggled = controller.toggleLightByScreenClick(width, height, width, height);
    ASSERT_FALSE(toggled);
    ASSERT_FALSE(controller.isLightEnabled());
    TEST_PASS;
}

int main() {
    std::cout << "=== MouseController Interface Unit Tests ===" << std::endl << std::endl;

    TestResult results[] = {
        test_default_state(),
        test_drag_updates_rotation(),
        test_release_stops_dragging(),
        test_rotation_api_rotate_by(),
        test_zoom_api_scroll_and_zoomby(),
        test_pitch_clamp_with_default_limits(),
        test_custom_pitch_limits(),
        test_custom_sensitivity(),
        test_reset_restores_view_state(),
        test_build_imouse_payload(),
        test_light_toggle_switch_hit(),
        test_light_toggle_switch_miss(),
    };

    int passed = 0;
    int total = sizeof(results) / sizeof(results[0]);
    for (const auto& result : results) {
        printTestResult(result);
        if (result.passed) {
            ++passed;
        }
    }

    std::cout << std::endl;
    std::cout << "Results: " << passed << "/" << total << " tests passed" << std::endl;
    return passed == total ? 0 : 1;
}
