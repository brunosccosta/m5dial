#include "Buzzer.h"
#include <M5Dial.h>

Buzzer buzzer;

void Buzzer::tick()    { if (!_muted) M5Dial.Speaker.tone(TICK_FREQ,    TICK_MS);    }
void Buzzer::tap()     { if (!_muted) M5Dial.Speaker.tone(TAP_FREQ,     TAP_MS);     }
void Buzzer::confirm() { if (!_muted) M5Dial.Speaker.tone(CONFIRM_FREQ, CONFIRM_MS); }
void Buzzer::wake()    { if (!_muted) M5Dial.Speaker.tone(WAKE_FREQ,    WAKE_MS);    }
