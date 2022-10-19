import Internal

class Log:
    @staticmethod
    def Trace(message):
        Internal.Log_Trace(message)

    @staticmethod
    def Info(message):
        Internal.Log_Info(message)

    @staticmethod
    def Warn(message):
        Internal.Log_Warn(message)

    @staticmethod
    def Error(message):
        Internal.Log_Error(message)
    
    @staticmethod
    def Fatal(message):
        Internal.Log_Fatal(message)
