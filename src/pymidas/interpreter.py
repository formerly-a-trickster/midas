
class Environment(object):
    def __init__(self, parent=None):
        self.parent = parent
        self.values = {}

    def __getitem__(self, item: str):
        if item in self.values:
            return self.values[item]
        elif self.parent is not None:
            return self.parent[item]
        else:
            raise "Unknown label %s" % item

