#include "FestoCmmsControl.h"
#include <Arduino.h>

FestoCmmsControl::FestoCmmsControl(int diEnableController,
                                   int diEnableMotor,
                                   int diDisableStop,
                                   int diStartMotion,
                                   int diRecordBit0,
                                   int diRecordBit1,
                                   int diRecordBit2,
                                   int diRecordBit3,
                                   int diRecordBit4,
                                   int doEnabled,
                                   int doMotionFinished,
                                   int doUnknown,
                                   int doError)
    : m_DiEnableController(diEnableController), m_DiEnableMotor(diEnableMotor), m_DiDisableStop(diDisableStop), m_DiStartMotion(diStartMotion), m_DiRecordBit0(diRecordBit0), m_DiRecordBit1(diRecordBit1), m_DiRecordBit2(diRecordBit2), m_DiRecordBit3(diRecordBit3), m_DiRecordBit4(diRecordBit4), m_DoEnabled(doEnabled), m_DoMotionFinished(doMotionFinished), m_DoUnknown(doUnknown), m_DoError(doError)
{
    SetRecordBits();
    Serial.printf("Enable controller: %i\n", m_DiEnableController);
    digitalWrite(m_DiEnableController, LOW);
    digitalWrite(m_DiEnableMotor, LOW);
    digitalWrite(m_DiDisableStop, LOW);
    digitalWrite(m_DiStartMotion, LOW);
    digitalWrite(m_DiRecordBit0, LOW);
    digitalWrite(m_DiRecordBit1, LOW);
    Serial.println("Festo controller created");
}

void FestoCmmsControl::EnableController()
{
    Serial.println("Enable controller");
    // SetController(true);
    
    digitalWrite(m_DiEnableMotor, true);
    digitalWrite(m_DiEnableController, true);
}

void FestoCmmsControl::DisableController()
{
    Serial.println("Disable controller");
    // SetController(false);
    
    digitalWrite(m_DiEnableMotor, false);
    digitalWrite(m_DiEnableController, false);
}

void FestoCmmsControl::StopMotion(bool stop)
{
    Serial.println("Stop motion");
    digitalWrite(m_DiStartMotion, !stop);
    digitalWrite(m_DiDisableStop, stop);
}

void FestoCmmsControl::Home(bool doHome)
{
    if (doHome)
    {
        Serial.println("Start homing");
        GoToPosition(0);
    }
    else
    {
        StopMotion(true);
    }
}

void FestoCmmsControl::GoToPosition(int posNr)
{
    // Only 63 positions known in internal memory
    if (posNr < 64 && !HasError())
    {
        Serial.printf("Check value: %i\n", posNr);

        // Check which bits are set
        for (int i = 0; i < 5; i++)
        {
            bool active = posNr & 1;
            Serial.printf("Index: %i -> Output: %i %s\n", i, m_RecordBits[i], active ? "active" : "inactive");

            // Set the record bit input
            digitalWrite(m_RecordBits[i], active ? HIGH : LOW);
            posNr = posNr >> 1;
        }

        digitalWrite(m_DiStartMotion, true);
        Serial.println("");
    }
    else
    {
        Serial.printf("Invalid pos nr: %i\n", posNr);
        StopMotion(true);
    }
}

bool FestoCmmsControl::HasError()
{
    bool hasError = digitalRead(m_DoError);
    Serial.printf("Controller has%s error\n", hasError ? " " : " no");
    return hasError;
}

void FestoCmmsControl::SetRecordBits()
{
    Serial.println("Record bit pins:");
    int recordBitPin;
    for (int i = 0; i < 5; i++)
    {
        switch (i)
        {
            case 0: recordBitPin = m_DiRecordBit0; break;
            case 1: recordBitPin = m_DiRecordBit1; break;
            case 2: recordBitPin = m_DiRecordBit2; break;
            case 3: recordBitPin = m_DiRecordBit3; break;
            case 4: recordBitPin = m_DiRecordBit4; break;
            default: return;
        }
        m_RecordBits[i] = recordBitPin;
        Serial.printf("  Pin nr: %i\n", recordBitPin);
    }
    Serial.println("Record bits set");
}

void FestoCmmsControl::SetController(bool enable)
{
    digitalWrite(m_DiEnableMotor, enable);
    digitalWrite(m_DiEnableController, enable);
}