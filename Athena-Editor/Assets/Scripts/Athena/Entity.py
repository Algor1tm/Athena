from Internal import Entity_FindEntityByName, Entity_GetScriptInstance, Entity_ExistsScriptInstance
from Internal import UUID
from Internal import Log


class Entity:
    def __init__(self, id = UUID(0)):
        self._ID = id

    def HasComponent(self, component):
        return component(self._ID)._HasThisComponent()

    def GetComponent(self, component):
        if not self.HasComponent(component):
            Log.Error(f"Entity does not have such component: '{type(component()).__name__}'")
            return None
        
        return component(self._ID)

    def AsScriptInstance(self):
        """
        Cast base class Entity to existing scripting instance
        If no exists - return self
        """
        if(Entity_ExistsScriptInstance(self._ID)):
            return Entity_GetScriptInstance(self._ID)

        return self;

    @staticmethod
    def FindEntityByName(name):
        """
        Finds Entity with name and tries to cast to script instance
        If not find return None
        """
        uuid = Entity_FindEntityByName(name)
        if uuid == UUID(0):
            Log.Error(f"Failed to find Entity with name: '{name}'")
            return None

        return Entity(uuid).AsScriptInstance()
