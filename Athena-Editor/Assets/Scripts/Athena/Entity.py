from Internal import Entity_FindEntityByName
from Internal import UUID


class Entity:
    def __init__(self, id = UUID(0)):
        self._ID = id

    def HasComponent(self, component):
        return component(self._ID)._HasThisComponent()

    def GetComponent(self, component):
        if not self.HasComponent(component):
            return None

        return component(self._ID)

    @staticmethod
    def FindEntityByName(self, name):
        uuid = Entity_FindEntityByName(name)
        if(uuid == 0):
            return None
        return Entity(uuid)
