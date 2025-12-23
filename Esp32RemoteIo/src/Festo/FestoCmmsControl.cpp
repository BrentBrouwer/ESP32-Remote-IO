#include "FestoCmmsControl.h"

FestoCmmsControl::FestoCmmsControl(int diEnableController, 
                         int diEnableMotor, 
                         int diDisableStop, 
                         int diStartMotion,
                         int diRecordBit0,
                         int diRecordBit1,
                         int doEnabled,
                         int doMotionFinished,
                         int doUnknown,
                         int doError)
: m_DiEnableController(diEnableController)
, m_DiEnableMotor(diEnableMotor)
, m_DiDisableStop(diDisableStop)
{

}

void FestoCmmsControl::EnableController()
{

}

void FestoCmmsControl::DisableController()
{

}

void FestoCmmsControl::StopMotion()
{

}

void FestoCmmsControl::Home()
{

}

void FestoCmmsControl::GoToPosition(int posNr)
{

}