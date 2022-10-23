from Athena_Core import *


class Camera(Entity):
    _Speed = 5
    _Position = Vector3(0, 0, 0)
    _CubicBody = None

    def OnCreate(self):
        Log.Warn(f"Camera::OnCreate() - {self._ID.AsUInt64()}")
        self._Position = self.GetComponent(TransformComponent).Translation
        self._CubicBody = Entity.FindEntityByName("YellowCubic").GetComponent(Rigidbody2DComponent)

    def OnUpdate(self, frameTime):
        if Input.IsKeyPressed(Keyboard.Space):
            Log.Error(f"SPACE PRESSED!!   FrameTime - {frameTime.AsSeconds()}")
            self._CubicBody.ApplyLinearImpulseToCenter(Vector2(0, frameTime.AsSeconds() * self._Speed / 5), True)

        offset = frameTime.AsSeconds() * self._Speed

        cubicPos = Entity.FindEntityByName("YellowCubic").GetComponent(TransformComponent).Translation
        self._Position.x = cubicPos.x
        self._Position.y = cubicPos.y

        if Input.IsKeyPressed(Keyboard.A):
            self._Position.x -= offset
        if Input.IsKeyPressed(Keyboard.W):
            self._Position.y += offset
        if Input.IsKeyPressed(Keyboard.S):
            self._Position.y -= offset
        if Input.IsKeyPressed(Keyboard.D):
            self._Position.x += offset
 