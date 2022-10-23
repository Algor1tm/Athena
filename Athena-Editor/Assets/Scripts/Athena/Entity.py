class Entity:
    _ID = 0

    def HasComponent(self, component):
        return component(self._ID)._HasThisComponent()

    def GetComponent(self, component):
        if not self.HasComponent(component):
            return None

        return component(self._ID)
