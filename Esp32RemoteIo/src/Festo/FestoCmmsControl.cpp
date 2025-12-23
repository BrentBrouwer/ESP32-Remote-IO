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
    digitalWrite(m_DiEnableController, LOW);
    digitalWrite(m_DiEnableMotor, LOW);
    digitalWrite(m_DiDisableStop, LOW);
    digitalWrite(m_DiStartMotion, LOW);
    digitalWrite(m_DiRecordBit0, LOW);
    digitalWrite(m_DiRecordBit1, LOW);
}

void FestoCmmsControl::EnableController()
{
    SetController(true);
}

void FestoCmmsControl::DisableController()
{
    SetController(false);
}

void FestoCmmsControl::StopMotion()
{
    digitalWrite(m_DiStartMotion, false);
    digitalWrite(m_DiDisableStop, true);
}

void FestoCmmsControl::Home()
{
    StopMotion();
    GoToPosition(0);
}

void FestoCmmsControl::GoToPosition(int posNr)
{
    // Only 63 positions known in internal memory
    if (posNr < 64 && !HasError())
    {
        Serial.printf("Check value: %i", posNr);

        // Check which bits are set
        for (int i = 0; i < 5; i++)
        {
            Serial.printf("Cycle: %i", i);
            posNr = posNr << 1;
            Serial.printf("Value: %i", posNr);
            bool active = posNr & 1;
            // Serial.printf("Bit: %s", active ? "active" : "inactive");
            Serial.printf("Output: %i %s", *m_RecordBits[i], active ? "active" : "inactive");

            // Set the record bit input
            digitalWrite(*m_RecordBits[i], active ? HIGH : LOW);
        }

        digitalWrite(m_DiStartMotion, true);
        Serial.println("");
    }
    else
    {
        StopMotion();
    }
}

bool FestoCmmsControl::HasError()
{
    return digitalRead(m_DoError);
}

void FestoCmmsControl::SetRecordBits()
{
    const int *recordBitAddr;
    for (int i = 0; i < 5; i++)
    {
        switch (i)
        {
        case 0:
            recordBitAddr = &m_DiRecordBit0;
        case 1:
            recordBitAddr = &m_DiRecordBit1;
        case 2:
            recordBitAddr = &m_DiRecordBit2;
        case 3:
            recordBitAddr = &m_DiRecordBit3;
        case 4:
            recordBitAddr = &m_DiRecordBit4;
        }
        m_RecordBits[i] = recordBitAddr;
    }

    Serial.println("Record bit pins:");
    for (int i = 0; i < 5; i++)
    {
        int value = *m_RecordBits[i];
        Serial.printf("  Pin nr: %i", *m_RecordBits[i]);
    }
}

void FestoCmmsControl::SetController(bool enable)
{
    digitalWrite(m_DiEnableMotor, enable);
    digitalWrite(m_DiEnableController, enable);
    digitalWrite(m_DiDisableStop, enable);
}