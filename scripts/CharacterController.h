#pragma once

#include "ScriptBase.h"
#include "Engine.h"
#include "ScriptProperty.h"
#include "Vector2.h"

class CharacterController : public doriax::ScriptBase {
public:
    DPROPERTY("Is Active")
    bool isActive = true;

    DPROPERTY("Movement Speed")
    float moveSpeed = 8.0f;

    DPROPERTY("Rotation Speed")
    float rotationSpeed = 15.0f;

    DPROPERTY("Jump Force")
    float jumpForce = 5.0f;

    DPROPERTY("Mouse Sensitivity")
    float mouseSensitivity = 0.2f;

    DPROPERTY("Camera Distance")
    float cameraDistance = 8.0f;

    DPROPERTY("Idle Animation")
    int idleAnimation = 1;

    DPROPERTY("Walk Animation")
    int walkAnimation = 3;

    DPROPERTY("Jump Animation")
    int jumpAnimation = 2;

    DPROPERTY("Waiting Idle Animation")
    int waitingIdleAnimation = 0;

    DPROPERTY("Waiting Idle Delay")
    float waitingIdleDelay = 5.0f;

    CharacterController(doriax::Scene* scene, doriax::Entity entity);
    ~CharacterController();

    void onUpdate();

private:
    int currentAnimState = -1;
    bool firstMouse = true;
    doriax::Vector2 lastMousePos;
    float camYaw = 0.0f;
    float camPitch = -30.0f;

    bool isJumping = false;
    float idleTime = 0.0f;
};
