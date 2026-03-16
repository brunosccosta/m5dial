#ifndef INPUT_H
#define INPUT_H

#include <Arduino.h>
#include <M5Dial.h>

class Input {
public:
    void begin();
    void update();
    
    int getEncoderDelta();  // Returns change since last call
    bool isButtonPressed();
    bool wasButtonPressed(); // Single press detection
    bool wasButtonReleased();
    
private:
    long lastEncoderValue;
    bool lastButtonState;
};

#endif
