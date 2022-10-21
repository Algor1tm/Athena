from Athena_Core import *


class Camera(Entity):
    _speed = 5

    def OnCreate(self):
        Log.Warn(f"CameraScript::OnCreate - {self._ID}")
        Log.Warn(f"PositionY: {self.GetTranslation().y}")

    def OnUpdate(self, frameTime):
        if Input.IsKeyPressed(Keyboard.Space):
            Log.Error(f"SPACE PRESSED!!   FrameTime - {frameTime}")

        offset = frameTime.AsSeconds() * self._speed

        if Input.IsKeyPressed(Keyboard.A):
            self.GetTranslation().x -= offset
        if Input.IsKeyPressed(Keyboard.W):
            self.GetTranslation().y += offset
        if Input.IsKeyPressed(Keyboard.S):
            self.GetTranslation().y -= offset
        if Input.IsKeyPressed(Keyboard.D):
            self.GetTranslation().x += offset
