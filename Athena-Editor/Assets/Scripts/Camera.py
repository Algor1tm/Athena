import Athena

class Camera(Athena.Entity):
    def OnCreate(self):
        Athena.Log.Info(f"CameraScript::OnCreate - {self._ID}")

    def OnUpdate(self, frameTime):
        Athena.Log.Warn("CameraScript::OnUpdate")
