#include "Input.h"

void Input::begin() {
    lastEncoderValue = M5Dial.Encoder.read();
    lastButtonState = false;
}

void Input::update() {
    // M5Dial.update() should be called before this in main loop
}

int Input::getEncoderDelta() {
    long currentValue = M5Dial.Encoder.read();
    int delta = currentValue - lastEncoderValue;
    lastEncoderValue = currentValue;
    return delta;
}

bool Input::isButtonPressed() {
    return M5Dial.BtnA.isPressed();
}

bool Input::wasButtonPressed() {
    return M5Dial.BtnA.wasPressed();
}

bool Input::wasButtonReleased() {
    return M5Dial.BtnA.wasReleased();
}
