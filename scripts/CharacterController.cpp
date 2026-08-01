#include "CharacterController.h"
#include "Input.h"
#include "Object.h"
#include "Model.h"
#include "Angle.h"
#include "Camera.h"
#include "Animation.h"
#include "Body3D.h"
#include <cmath>
#include <algorithm>

using namespace doriax;

CharacterController::CharacterController(Scene* scene, Entity entity): ScriptBase(scene, entity) {
    REGISTER_ENGINE_EVENT(onUpdate);
}

CharacterController::~CharacterController() {
    UNREGISTER_ENGINE_EVENT(onUpdate);
}

void CharacterController::onUpdate() {
    if (!isActive) return;

    Object obj(getScene(), getEntity());
    Model model(getScene(), getEntity());
    Body3D body(getScene(), getEntity());
    float deltaTime = Engine::getDeltatime();

    Entity camEntity = getScene()->getCamera();
    if (camEntity == NULL_ENTITY) return;

    Camera cam(getScene(), camEntity);

    int gamepadId = -1;
    if (Input::numGamepads() > 0) {
        gamepadId = Input::getGamepadId(0);
    }

    Vector2 mousePos = Input::getMousePosition();
    if (firstMouse) {
        lastMousePos = mousePos;
        firstMouse = false;
    }

    if (Input::isMousePressed(D_MOUSE_BUTTON_LEFT)) {
        float xoffset = mousePos.x - lastMousePos.x;
        float yoffset = mousePos.y - lastMousePos.y;

        camYaw -= xoffset * mouseSensitivity;
        camPitch -= yoffset * mouseSensitivity;
    }

    if (gamepadId != -1 && Input::isGamepadConnected(gamepadId)) {
        float rightX = Input::getGamepadAxis(gamepadId, D_GAMEPAD_AXIS_RIGHT_X);
        float rightY = Input::getGamepadAxis(gamepadId, D_GAMEPAD_AXIS_RIGHT_Y);
        const float lookDeadzone = 0.2f;

        if (std::abs(rightX) < lookDeadzone) rightX = 0.0f;
        if (std::abs(rightY) < lookDeadzone) rightY = 0.0f;

        camYaw -= rightX * mouseSensitivity * 12.0f;
        camPitch += rightY * mouseSensitivity * 12.0f;
    }

    if (camPitch > 89.0f) camPitch = 89.0f;
    if (camPitch < -89.0f) camPitch = -89.0f;

    lastMousePos = mousePos;

    Quaternion camRot;
    camRot.fromEulerAngles(camPitch, camYaw, 0.0f, RotationOrder::YXZ);
    cam.setRotation(camRot);

    Vector3 targetPos = obj.getPosition() + Vector3(0, 1.5f, 0);
    Vector3 camForward = camRot * Vector3(0, 0, -1);

    cam.setPosition(targetPos - camForward * cameraDistance);
    cam.setTarget(targetPos);

    Vector3 camRight = camRot * Vector3(1, 0, 0);

    camForward.y = 0;
    camRight.y = 0;
    if (camForward.length() > 0.001f) camForward.normalize();
    if (camRight.length() > 0.001f) camRight.normalize();

    Vector3 velocity = body.getLinearVelocity();

    if (std::abs(velocity.y) < 0.1f) {
        isJumping = false;
    } else {
        isJumping = true;
    }

    bool jumpPressed = Input::isKeyPressed(D_KEY_SPACE);
    if (gamepadId != -1 && Input::isGamepadConnected(gamepadId)) {
        jumpPressed = jumpPressed || Input::isGamepadButtonPressed(gamepadId, D_GAMEPAD_BUTTON_A);
    }

    if (!isJumping && jumpPressed) {
        velocity.y = jumpForce;
        isJumping = true;
    }

    bool moveForward = Input::isKeyPressed(D_KEY_W) || Input::isKeyPressed(D_KEY_UP);
    bool moveBackward = Input::isKeyPressed(D_KEY_S) || Input::isKeyPressed(D_KEY_DOWN);
    bool moveLeft = Input::isKeyPressed(D_KEY_A) || Input::isKeyPressed(D_KEY_LEFT);
    bool moveRight = Input::isKeyPressed(D_KEY_D) || Input::isKeyPressed(D_KEY_RIGHT);

    float gamepadX = 0.0f;
    float gamepadY = 0.0f;
    if (gamepadId != -1 && Input::isGamepadConnected(gamepadId)) {
        gamepadX = Input::getGamepadAxis(gamepadId, D_GAMEPAD_AXIS_LEFT_X);
        gamepadY = Input::getGamepadAxis(gamepadId, D_GAMEPAD_AXIS_LEFT_Y);

        if (Input::isGamepadButtonPressed(gamepadId, D_GAMEPAD_BUTTON_DPAD_UP)) gamepadY = -1.0f;
        if (Input::isGamepadButtonPressed(gamepadId, D_GAMEPAD_BUTTON_DPAD_DOWN)) gamepadY = 1.0f;
        if (Input::isGamepadButtonPressed(gamepadId, D_GAMEPAD_BUTTON_DPAD_LEFT)) gamepadX = -1.0f;
        if (Input::isGamepadButtonPressed(gamepadId, D_GAMEPAD_BUTTON_DPAD_RIGHT)) gamepadX = 1.0f;

        const float moveDeadzone = 0.2f;
        if (std::abs(gamepadX) < moveDeadzone) gamepadX = 0.0f;
        if (std::abs(gamepadY) < moveDeadzone) gamepadY = 0.0f;
    }

    bool hasMoveInput = false;
    Vector3 inputDir(0, 0, 0);

    if (moveForward) {
        inputDir = inputDir + camForward;
        hasMoveInput = true;
    }
    if (moveBackward) {
        inputDir = inputDir - camForward;
        hasMoveInput = true;
    }
    if (moveLeft) {
        inputDir = inputDir - camRight;
        hasMoveInput = true;
    }
    if (moveRight) {
        inputDir = inputDir + camRight;
        hasMoveInput = true;
    }

    if (gamepadY < 0.0f) {
        inputDir = inputDir + camForward * (-gamepadY);
        hasMoveInput = true;
    }
    if (gamepadY > 0.0f) {
        inputDir = inputDir - camForward * gamepadY;
        hasMoveInput = true;
    }
    if (gamepadX < 0.0f) {
        inputDir = inputDir - camRight * (-gamepadX);
        hasMoveInput = true;
    }
    if (gamepadX > 0.0f) {
        inputDir = inputDir + camRight * gamepadX;
        hasMoveInput = true;
    }

    if (inputDir.length() > 0.1f) {
        inputDir.normalize();

        velocity.x = inputDir.x * moveSpeed;
        velocity.z = inputDir.z * moveSpeed;

        float targetAngle = Angle::radToDefault(std::atan2(inputDir.x, inputDir.z));
        Quaternion targetRot(0.0f, targetAngle, 0.0f);

        float t = std::min(rotationSpeed * deltaTime, 1.0f);

        body.setAngularVelocityClamped(Vector3::ZERO);
        obj.setRotation(Quaternion::slerp(t, obj.getRotation(), targetRot));
        idleTime = 0.0f;
    } else {
        velocity.x = 0;
        velocity.z = 0;
        if (!isJumping) {
            idleTime += deltaTime;
        }
    }

    if (isJumping || hasMoveInput) {
        idleTime = 0.0f;
    }

    body.setLinearVelocity(velocity);

    int desiredAnimState = 0;
    if (isJumping) {
        desiredAnimState = 2;
    }
    else if (hasMoveInput) {
        desiredAnimState = 1;
    }
    else if (idleTime >= waitingIdleDelay) {
        desiredAnimState = 3;
    }
    else {
        desiredAnimState = 0;
    }

    if (desiredAnimState != currentAnimState) {
        currentAnimState = desiredAnimState;
        float fadeTime = 0.1f;

        if (currentAnimState == 0) {
            Animation anim = model.getAnimation(idleAnimation);
            anim.setLoop(true);
            model.playAnimation(idleAnimation, fadeTime);
        }
        else if (currentAnimState == 1) {
            Animation anim = model.getAnimation(walkAnimation);
            anim.setLoop(true);
            model.playAnimation(walkAnimation, fadeTime);
        }
        else if (currentAnimState == 2) {
            Animation anim = model.getAnimation(jumpAnimation);
            anim.setLoop(false);
            model.playAnimation(jumpAnimation, fadeTime);
        }
        else if (currentAnimState == 3) {
            Animation anim = model.getAnimation(waitingIdleAnimation);
            anim.setLoop(true);
            model.playAnimation(waitingIdleAnimation, fadeTime);
        }
    }
}
