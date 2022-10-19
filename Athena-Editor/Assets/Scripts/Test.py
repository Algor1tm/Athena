import Athena


Athena.Log.Fatal("Import Test")
entity = Athena.Entity()
Athena.Input.IsKeyPressed(Athena.Keyboard.Up)
Athena.Log.Warn(Athena.Input.GetMousePosition().__repr__())
