#include "FestoCmmsControl.h"
#include "../Logging/Logging.h"
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
    : m_DiEnableController(diEnableController), m_DiEnableMotor(diEnableMotor), m_DiDisableStop(diDisableStop), m_DiStartMotion(diStartMotion), m_DiRecordBit0(diRecordBit0), m_DiRecordBit1(diRecordBit1), m_DiRecordBit2(diRecordBit2), m_DiRecordBit3(diRecordBit3), m_DiRecordBit4(diRecordBit4), m_DoControllerEnabled(doEnabled), m_DoMotionFinished(doMotionFinished), m_DoAcknowledgeStart(doUnknown), m_DoError(doError)
{
    PrintPins();
    SetRecordBits();
    AllOff();
    LogWithTime("Festo controller created");
}

void FestoCmmsControl::EnableController()
{
    SetController(true);
}

void FestoCmmsControl::DisableController()
{
    SetController(false);
}

// void FestoCmmsControl::StopMotion()
// {
//     LogWithTime("Stop motion");
//     digitalWrite(m_DiStartMotion, false);
//     // digitalWrite(m_DiDisableStop, false);
// }

void FestoCmmsControl::Home()
{
    LogWithTime("Start homing");
    GoToPosition(0);
}

void FestoCmmsControl::GoToPosition(int posNr)
{
    // Only 63 positions known in internal memory
    if (posNr < 64) // && !HasError())
    {
        LogWithTime("[Request] Go to position: %i", posNr);

        // Check which bits are set
        for (int i = 0; i < 5; i++)
        {
            bool active = posNr & 1;
            // Serial.printf("Index: %i -> Output: %i %s\n", i, m_RecordBits[i], active ? "active" : "inactive");

            // Set the record bit input
            digitalWrite(m_RecordBits[i], active ? HIGH : LOW);
            posNr = posNr >> 1;
        }

        // digitalWrite(m_DiDisableStop, true);
        delay(50);
        digitalWrite(m_DiStartMotion, true);
        delay(50);
        digitalWrite(m_DiStartMotion, false);
        // Serial.println("");
    }
    else
    {
        LogWithTime("Invalid pos nr: %i\n", posNr);
        // StopMotion();
    }
}

bool FestoCmmsControl::IsControllerReady()
{
    bool value = digitalRead(m_DoControllerEnabled);
    if (m_ControllerEnabled != value)
    {
        m_ControllerEnabled = value;
        LogWithTime("[Status] Controller %s\n", m_ControllerEnabled ? "ready" : "not ready");
    }
    return value;
}

bool FestoCmmsControl::HasError()
{
    bool value = digitalRead(m_DoError);
    if (m_ErrorActive != value)
    {
        m_ErrorActive = value;
        LogWithTime("[Status] Controller has%serror\n", m_ErrorActive ? " " : " no ");
    }
    return value;
}

bool FestoCmmsControl::IsMotionFinished()
{
    return m_MotionFinishedWithDelay;
}

bool FestoCmmsControl::IsStartAcknowledged()
{
    bool value = digitalRead(m_DoAcknowledgeStart);
    if (m_AcknowledgeStart != value)
    {
        m_AcknowledgeStart = value;
        LogWithTime("[Status] Acknowledge start %s\n", m_AcknowledgeStart ? "active" : "not active");
    }
    return value;
}

void FestoCmmsControl::AllOff()
{
    digitalWrite(m_DiEnableController, LOW);
    digitalWrite(m_DiEnableMotor, LOW);
    digitalWrite(m_DiDisableStop, LOW);
    digitalWrite(m_DiStartMotion, LOW);
    digitalWrite(m_DiRecordBit0, LOW);
    digitalWrite(m_DiRecordBit1, LOW);
    digitalWrite(m_DiRecordBit2, LOW);
    digitalWrite(m_DiRecordBit3, LOW);
    digitalWrite(m_DiRecordBit4, LOW);
}

void FestoCmmsControl::CheckStates()
{
    unsigned long now = millis();
    // IsControllerReady();

    // Motion finish
    bool value = digitalRead(m_DoMotionFinished);
    if (m_MotionFinished != value)
    {
        m_MotionFinished = value;
        if (value)
        {
            // Rising edge
            m_MotionFinishedMs = now;
        }
        else
        {
            // Falling edge
            m_MotionFinishedWithDelay = false;
            m_MotionStartedMs = now;
        }
    }
    else
    {
        // Same status, check for how long
        if (m_MotionFinished &&
            !m_MotionFinishedWithDelay &&
            now - m_MotionFinishedMs >= 50)
        {
            m_MotionFinishedWithDelay = true;
            LogWithTime("[Status] Motion %s\n", m_MotionFinished ? "finished" : "not finished");
        }
        // else if (!m_MotionFinished &&
        //          now - m_MotionStartedMs >= 50)
        // {
        //     m_MotionFinishedWithDelay = false;
        //     LogWithTime("[Status] Motion %s\n", m_MotionFinished ? "finished" : "not finished");
        // }
    }

    // HasError();
}

void FestoCmmsControl::PrintPins()
{
    Serial.println("Pin allocation");
    Serial.printf("Enable controller: %i\n", m_DiEnableController);
    Serial.printf("Enable motor: %i\n", m_DiEnableMotor);
    Serial.printf("Disable stop: %i\n", m_DiDisableStop);
    Serial.printf("Start motion: %i\n", m_DiStartMotion);
    Serial.printf("Record bit 0: %i\n", m_DiRecordBit0);
    Serial.printf("Record bit 1: %i\n", m_DiRecordBit1);
    Serial.printf("Record bit 2: %i\n", m_DiRecordBit2);
    Serial.printf("Record bit 3: %i\n", m_DiRecordBit3);
    Serial.printf("Record bit 4: %i\n", m_DiRecordBit4);

    Serial.printf("DO controller enabled: %i\n", m_DoControllerEnabled);
    Serial.printf("DO motion finished: %i\n", m_DoMotionFinished);
    Serial.printf("DO acknowledge start: %i\n", m_DoAcknowledgeStart);
    Serial.printf("DO error: %i\n", m_DoError);
}

void FestoCmmsControl::SetRecordBits()
{
    // Serial.println("Record bit pins:");
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
        // Serial.printf("  Pin nr: %i\n", recordBitPin);
    }
    LogWithTime("Record bits set");
}

void FestoCmmsControl::SetController(bool enable)
{
    LogWithTime("[Request] %s controller\n", enable ? "Enable" : "Disable");
    digitalWrite(m_DiDisableStop, enable);
    digitalWrite(m_DiEnableMotor, enable);
    delay(15);
    digitalWrite(m_DiEnableController, enable);
}

void FestoCmmsControl::WaitForControllerReady()
{
    while (!IsControllerReady())
    {
        delay(10);
    }
}

void FestoCmmsControl::WaitForMotionFinish()
{
    while (!digitalRead(m_DoMotionFinished))
    {
        delay(10);
    }
}