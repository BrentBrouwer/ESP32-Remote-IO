
class FestoCmmsControl
{
    public:
        // Constructor
        FestoCmmsControl(int diEnableController, 
                         int diEnableMotor, 
                         int diDisableStop, 
                         int diStartMotion,
                         int diRecordBit0,
                         int diRecordBit1,
                         int doEnabled,
                         int doMotionFinished,
                         int doTest,
                         int doError);

        // Methods
        void EnableController();
        void DisableController();
        void StopMotion();
        void Home();
        void GoToPosition(int posNr);

    private:
        // CMMS Inputs
        int m_DiEnableController;
        int m_DiEnableMotor;
        int m_DiDisableStop; 
        int m_DiStartMotion; 
        int m_DiRecordBit0;
        int m_DiRecordBit1;

        // CMMS Outputs
        int m_DoEnabled;
        int m_DoMotionFinished;
        int m_DoTest;
        int m_DoError;
};
