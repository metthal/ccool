class Sequence:
    def __init__(self, *patterns):
        self.patterns = list(patterns)

    def to_regex(self):
        return "-".join([p.to_regex() for p in self.patterns])

    def __repr__(self):
        return str(self)

    def __str__(self):
        return "-".join([str(p) for p in self.patterns])



class Repeats:
    def __init__(self, pattern, min=None, max=None):
        self.pattern = pattern
        self.min = min
        self.max = max

    def to_regex(self):
        if self.min is None and self.max is None:
            rep = "*"
        elif self.min is not None and self.max is not None and self.min == self.max:
            rep = f"{{{self.min}}}"
        else:
            rep = "{{{},{}}}".format(self.min or "", self.max or "")
        return "({}(-|$)){}".format(self.pattern.to_regex(), rep)

    def __repr__(self):
        return str(self)

    def __str__(self):
        return "repeat({}, min={}, max={})".format(str(self.pattern), self.min, self.max)


class AnyCall:
    def to_regex(self):
        return f"<[^>]+>"


class Call:
    def __init__(self, name: str, **kwargs):
        self.name = name
        self.args = kwargs

    def to_regex(self):
        args = ",".join([f"{key}={value}" for key, value in sorted(self.args.items())])
        return fr"<{self.name}\({args}\)>"

    def __repr__(self):
        return str(self)

    def __str__(self):
        args = ",".join([f"{key}={value}" for key, value in sorted(self.args.items())])
        return f"<{self.name}({args})>"
