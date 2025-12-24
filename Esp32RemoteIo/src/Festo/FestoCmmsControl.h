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
                         int diRecordBit2,
                         int diRecordBit3,
                         int diRecordBit4,
                         int doEnabled,
                         int doMotionFinished,
                         int doTest,
                         int doError);

        // Methods
        void EnableController();
        void DisableController();
        void StopMotion(bool stop);
        void Home(bool doHome);
        void GoToPosition(int posNr);
        bool HasError();

    private:
        // CMMS Inputs
        const int m_DiEnableController;
        const int m_DiEnableMotor;
        const int m_DiDisableStop; 
        const int m_DiStartMotion; 
        const int m_DiRecordBit0;
        const int m_DiRecordBit1;
        const int m_DiRecordBit2;
        const int m_DiRecordBit3;
        const int m_DiRecordBit4;
        int m_RecordBits[5];

        // CMMS Outputs
        const int m_DoEnabled;
        const int m_DoMotionFinished;
        const int m_DoUnknown;
        const int m_DoError;

        // Methods
        void SetRecordBits();
        void SetController(bool enable);
};
