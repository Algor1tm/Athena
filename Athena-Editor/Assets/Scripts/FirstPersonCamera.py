from Athena_Core import *

def Sign(value):
    if(value > 0):
        return 1
    elif(value < 0):
        return -1
    
    return 0


class FirstPersonCamera(Entity):
    _initialMousePosition = Vector2()
    yaw = 0.0
    pitch = 0.0

    moveSpeed = 50.0
    rotationSpeed = 0.002 

    def OnCreate(self):
        Log.Warn(f"FirstPersonCamera::OnCreate() - {self._ID.AsUInt64()}")

    def OnUpdate(self, frameTime):
        delta = self.UpdateMousePosition()
        moveDirection = Vector3(0.0)

        if(Input.IsMouseButtonPressed(Mouse.Right)):
            yawSign = Sign(self.GetUpDirection().y)
            self.yaw += yawSign * delta.x
            self.pitch += delta.y

            if (Input.IsKeyPressed(Keyboard.W)):
                moveDirection += self.GetForwardDirection()
            elif (Input.IsKeyPressed(Keyboard.S)):
                moveDirection += -self.GetForwardDirection()

            elif (Input.IsKeyPressed(Keyboard.A)):
                moveDirection += -self.GetRightDirection()
            elif (Input.IsKeyPressed(Keyboard.D)):
                moveDirection += self.GetRightDirection()

            elif (Input.IsKeyPressed(Keyboard.E)):
                moveDirection += Vector3.up / 1.5
            elif (Input.IsKeyPressed(Keyboard.Q)):
                moveDirection += Vector3.down / 1.5

        transform = self.GetComponent(TransformComponent)
        transform.translation += moveDirection * frameTime.AsSeconds() * self.moveSpeed
        transform.rotation = self.GetOrientation()
        
        
    def UpdateMousePosition(self):
        mousePos = Input.GetMousePosition()
        delta = mousePos - self._initialMousePosition
        self._initialMousePosition = mousePos
        delta *= self.rotationSpeed
        return delta

    def GetUpDirection(self):
        return self.GetOrientation() * Vector3.up
    
    def GetRightDirection(self):
        return self.GetOrientation() * Vector3.right
    
    def GetForwardDirection(self):
        return self.GetOrientation() * Vector3.forward
    
    def GetOrientation(self):
        return Quaternion(Vector3(-self.pitch, -self.yaw, 0))
