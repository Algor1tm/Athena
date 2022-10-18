ATHENA_MODULE_FOLDER =  "" # "../../" # use in shipped application

import sys
sys.path.insert(0, ATHENA_MODULE_FOLDER)

from AthenaScriptCore import *

class Entity:
    def __init__(self):
        print("Entity()")
        pass

    def OnUpdate(self, frameTime):
        pass

    def OnEvent(self, event):
        pass
