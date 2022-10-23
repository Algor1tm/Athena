from Athena_Core import *


class Camera(Entity):
    _speed = 5
    position = Vector3(0, 0, 0)
    cubicPosition = Vector3(0, 0, 0)

    def OnCreate(self):
        Log.Warn(f"CameraScript::OnCreate - {self._ID}")
        self.position = self.GetComponent(TransformComponent).Translation
        self.cubicPosition = self.FindEntityByName("YellowCubic").GetComponent(TransformComponent).Translation

    def OnUpdate(self, frameTime):
        if Input.IsKeyPressed(Keyboard.Space):
            Log.Error(f"SPACE PRESSED!!   FrameTime - {frameTime.AsSeconds()}")
            self.cubicPosition.y += frameTime.AsSeconds() * self._speed / 2

        offset = frameTime.AsSeconds() * self._speed

        if Input.IsKeyPressed(Keyboard.A):
            self.position.x -= offset
        if Input.IsKeyPressed(Keyboard.W):
            self.position.y += offset
        if Input.IsKeyPressed(Keyboard.S):
            self.position.y -= offset
        if Input.IsKeyPressed(Keyboard.D):
            self.position.x += offset
