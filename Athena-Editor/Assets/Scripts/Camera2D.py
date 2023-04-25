from Athena_Core import *


class Camera2D(Entity):
    _translation = Vector3(0, 0, 0)

    def OnCreate(self):
        Log.Warn(f"Camera2D::OnCreate() - {self._ID.AsUInt64()}")

    def OnUpdate(self, frameTime):
        translation = self.GetComponent(TransformComponent).translation
        translation.x = self._translation.x
        translation.y = self._translation.y

    def SetPosition(self, translation):
        self._translation = translation
