"""Read an EMTSim netlist file into a list of elements.

Format, one element per line (node 0 = ground; '#' starts a comment):
    R  from to  ohm
    L  from to  henry
    C  from to  farad
    V  from to  magnitude(peak V) angle(deg) frequency(Hz) [resistance]   no resistance = ideal source
    SW from to  closed|open  t1 [t2 ...]     toggles state at each time (s)
    F  from to  t_apply [t_clear]
"""
import elements as el


def load(path):
    elems = []
    with open(path) as f:
        for lineno, raw in enumerate(f, 1):
            tok = raw.split("#", 1)[0].split()
            if not tok:
                continue
            where = "%s:%d" % (path, lineno)
            if len(tok) < 4:
                raise ValueError("%s: expected 'TYPE from to ...'" % where)
            kind, a, b, p = tok[0].upper(), int(tok[1]), int(tok[2]), tok[3:]
            if kind in ("R", "L", "C"):
                elems.append(el.RLC(brnType=kind, strnode=a, stpnode=b, value=float(p[0])))
            elif kind == "V":
                if len(p) < 3:
                    raise ValueError("%s: V needs magnitude angle frequency [resistance]" % where)
                elems.append(el.Source(brnType="S", strnode=a, stpnode=b, magnitude=float(p[0]),
                                       angle=float(p[1]), frequency=float(p[2]),
                                       resistance=float(p[3]) if len(p) > 3 else None))
            elif kind == "SW":
                if p[0] not in ("closed", "open"):
                    raise ValueError("%s: SW state must be 'closed' or 'open'" % where)
                elems.append(el.Switch(strnode=a, stpnode=b, closed=(p[0] == "closed"),
                                       t_switch=[float(x) for x in p[1:]]))
            elif kind == "F":
                elems.append(el.Fault(strnode=a, stpnode=b, t_apply=float(p[0]),
                                      t_clear=float(p[1]) if len(p) > 1 else None))
            else:
                raise ValueError("%s: unknown element type '%s'" % (where, tok[0]))
    return elems
