/*
 * EMTSim schematic model -> netlist.
 *
 * Used by the circuit editor in the browser (window.Schematic) and by the
 * tests under Node (require('./schematic.js')).
 *
 * Model (all coordinates in grid units):
 *   { v: 1, phases: 1 | 3,
 *     components: [{ id, type, name, x, y, rot, params: { ... strings ... } }],
 *     wires:      [{ x1, y1, x2, y2 }] }
 *
 * Two-terminal components run from terminal a = (x, y) to terminal b, LEN grid
 * units away in the direction given by rot (0 = right, 90 = down, 180 = left,
 * 270 = up). Terminal a is the netlist "from" node, b the "to" node; a voltage
 * source raises b above a. GND and the 3-phase fault F3 have a single terminal
 * at (x, y).
 *
 * Three-phase mode (phases: 3): wires and nodes carry phases a, b, c. Drawing
 * node k becomes netlist nodes 3k-2, 3k-1, 3k (phases a, b, c); ground is node 0
 * for every phase. Each 3-phase component is written as three single-phase
 * elements, one per phase, so the simulators need no changes. There is no
 * mutual coupling between phases.
 */
(function (root, factory) {
  if (typeof module === 'object' && module.exports) module.exports = factory();
  else root.Schematic = factory();
})(typeof self !== 'undefined' ? self : this, function () {
  'use strict';

  const LEN = 4;
  const DIRS = { 0: [1, 0], 90: [0, 1], 180: [-1, 0], 270: [0, -1] };
  const PH = ['a', 'b', 'c'];
  const STATE = { kind: 'select', options: ['closed', 'open'], def: 'closed' };
  const POLES = { key: 'mode', kind: 'select', options: ['together', 'independent'], def: 'together' };

  const TYPES = {
    R: { label: 'Resistor', prefix: 'R', params: [
      { key: 'value', label: 'Resistance', unit: 'Ω', def: '10' }] },
    L: { label: 'Inductor', prefix: 'L', params: [
      { key: 'value', label: 'Inductance', unit: 'H', def: '10m' }] },
    C: { label: 'Capacitor', prefix: 'C', params: [
      { key: 'value', label: 'Capacitance', unit: 'F', def: '10u' }] },
    V: { label: 'Voltage source', prefix: 'V', params: [
      { key: 'mag', label: 'Peak voltage', unit: 'V', def: '325' },
      { key: 'angle', label: 'Angle', unit: '°', def: '0' },
      { key: 'freq', label: 'Frequency', unit: 'Hz', def: '50' },
      { key: 'r', label: 'Internal resistance (blank or 0 = ideal)', unit: 'Ω', def: '0.1', optional: true }] },
    SW: { label: 'Switch', prefix: 'SW', params: [
      Object.assign({ key: 'state', label: 'Initial state' }, STATE),
      { key: 'times', label: 'Switching times (toggles at each)', unit: 's', kind: 'list', def: '50m', optional: true }] },
    F: { label: 'Fault', prefix: 'F', params: [
      { key: 't_apply', label: 'Applied at', unit: 's', def: '20m' },
      { key: 't_clear', label: 'Cleared at (blank = permanent)', unit: 's', def: '40m', optional: true }] },
    GND: { label: 'Ground', prefix: 'GND', params: [] },

    // ---- three-phase
    R3: { label: 'Resistor (3-phase)', prefix: 'R', three: true, base: 'R', params: [
      { key: 'value', label: 'Resistance per phase', unit: 'Ω', def: '10' }] },
    L3: { label: 'Inductor (3-phase)', prefix: 'L', three: true, base: 'L', params: [
      { key: 'value', label: 'Inductance per phase', unit: 'H', def: '10m' }] },
    C3: { label: 'Capacitor (3-phase)', prefix: 'C', three: true, base: 'C', params: [
      { key: 'value', label: 'Capacitance per phase', unit: 'F', def: '10u' }] },
    V3: { label: 'Voltage source (3-phase)', prefix: 'V', three: true, params: [
      { key: 'mag', label: 'Peak phase voltage (line to ground)', unit: 'V', def: '325' },
      { key: 'angle', label: 'Angle of phase a (b = a − 120°, c = a + 120°)', unit: '°', def: '0' },
      { key: 'freq', label: 'Frequency', unit: 'Hz', def: '50' },
      { key: 'r', label: 'Internal resistance per phase (blank or 0 = ideal)', unit: 'Ω', def: '0.1', optional: true }] },
    SW3: { label: 'Switch (3-phase)', prefix: 'SW', three: true, params: [
      Object.assign({}, POLES, { label: 'Poles' }),
      Object.assign({ key: 'state', label: 'Initial state', when: 'together' }, STATE),
      { key: 'times', label: 'Switching times (toggles at each)', unit: 's', kind: 'list', def: '50m', optional: true, when: 'together' },
    ].concat(...PH.map((p) => [
      Object.assign({ key: 'state_' + p, label: 'Pole ' + p + ': initial state', when: 'independent' }, STATE),
      { key: 'times_' + p, label: 'Pole ' + p + ': switching times', unit: 's', kind: 'list', def: '50m', optional: true, when: 'independent' }])) },
    F3: { label: 'Fault (3-phase)', prefix: 'F', three: true, single: true, params: [
      { key: 'kind', label: 'Fault type (-g = to ground)', kind: 'select',
        options: ['a-g', 'b-g', 'c-g', 'ab', 'bc', 'ca', 'ab-g', 'bc-g', 'ca-g', 'abc', 'abc-g'], def: 'a-g' },
      Object.assign({}, POLES, { label: 'Timing of the phases' }),
      { key: 't_apply', label: 'Applied at', unit: 's', def: '20m', when: 'together' },
      { key: 't_clear', label: 'Cleared at (blank = permanent)', unit: 's', def: '40m', optional: true, when: 'together' },
    ].concat(...PH.map((p) => [
      { key: 't_apply_' + p, label: 'Phase ' + p + ': applied at', unit: 's', def: '20m', when: 'independent' },
      { key: 't_clear_' + p, label: 'Phase ' + p + ': cleared at (blank = permanent)', unit: 's', def: '40m', optional: true, when: 'independent' }])) },
  };

  // Single-phase <-> three-phase equivalents (used when the drawing's mode is changed)
  const TO3 = { R: 'R3', L: 'L3', C: 'C3', V: 'V3', SW: 'SW3', F: 'F3' };
  const TO1 = { R3: 'R', L3: 'L', C3: 'C', V3: 'V', SW3: 'SW', F3: 'F' };
  const isThree = (type) => !!(TYPES[type] && TYPES[type].three);

  // Parameters shown/used for a component: those whose 'when' matches its pole/timing mode
  function activeParams(c) {
    const mode = (c.params && c.params.mode) || 'together';
    return TYPES[c.type].params.filter((p) => !p.when || p.when === mode);
  }

  // ------------------------------------------------------------ values

  const SI = { f: 1e-15, p: 1e-12, n: 1e-9, u: 1e-6, 'µ': 1e-6, m: 1e-3, k: 1e3, M: 1e6, meg: 1e6, G: 1e9 };

  // "3m" -> 0.003, "30uF" -> 3e-5, "1.5k" -> 1500, "20ms" -> 0.02. Empty -> null, invalid -> NaN.
  function parseValue(s) {
    s = String(s == null ? '' : s).trim();
    if (!s) return null;
    const m = s.match(/^([-+]?(?:\d+\.?\d*|\.\d+)(?:[eE][-+]?\d+)?)\s*(meg|[fpnuµmkMG])?\s*[A-Za-zΩ°µ]*$/);
    if (!m) return NaN;
    return parseFloat(m[1]) * (m[2] ? SI[m[2]] : 1);
  }

  function parseList(s) {
    const parts = String(s == null ? '' : s).split(/[,;\s]+/).filter(Boolean);
    return parts.map(parseValue);
  }

  // Netlist number format: up to 12 significant digits, shortest form
  function fmt(x) {
    return String(Number(x.toPrecision(12)));
  }

  // Display format with SI prefix: 0.003, 'H' -> "3 mH"
  function formatSI(x, unit) {
    if (x == null || !isFinite(x)) return '?';
    if (x === 0) return '0' + (unit ? ' ' + unit : '');
    const prefixes = [[1e9, 'G'], [1e6, 'M'], [1e3, 'k'], [1, ''], [1e-3, 'm'], [1e-6, 'µ'], [1e-9, 'n'], [1e-12, 'p']];
    const a = Math.abs(x);
    let p = prefixes.find(([f]) => a >= f * 0.9999) || prefixes[prefixes.length - 1];
    if (unit === '°' || unit === 'Hz' && a < 1e3) p = [1, ''];
    return String(Number((x / p[0]).toPrecision(4))) + ' ' + p[1] + (unit || '');
  }

  // ------------------------------------------------------------ geometry

  const singleTerminal = (type) => type === 'GND' || !!(TYPES[type] && TYPES[type].single);

  function terminals(c) {
    if (singleTerminal(c.type)) return [[c.x, c.y]];
    const d = DIRS[c.rot || 0];
    return [[c.x, c.y], [c.x + d[0] * LEN, c.y + d[1] * LEN]];
  }

  const key = (p) => p[0] + ',' + p[1];

  // Is point p on segment w (including the end points)?
  function onSegment(p, w) {
    const [x, y] = p;
    const cross = (w.x2 - w.x1) * (y - w.y1) - (w.y2 - w.y1) * (x - w.x1);
    if (Math.abs(cross) > 1e-9) return false;
    return x >= Math.min(w.x1, w.x2) && x <= Math.max(w.x1, w.x2) &&
           y >= Math.min(w.y1, w.y2) && y <= Math.max(w.y1, w.y2);
  }

  // ------------------------------------------------------------ connectivity

  // Groups all connection points into nets. Returns
  //   { pointNet: Map(key -> net index), nets: [{ points: [[x,y]...], terminals: n, ground: bool }],
  //     degree: Map(key -> connection count) }
  function connectivity(model) {
    const parent = new Map();
    const coords = new Map();
    const find = (k) => {
      while (parent.get(k) !== k) { parent.set(k, parent.get(parent.get(k))); k = parent.get(k); }
      return k;
    };
    const add = (p) => { const k = key(p); if (!parent.has(k)) { parent.set(k, k); coords.set(k, p); } return k; };
    const union = (a, b) => { const ra = find(a), rb = find(b); if (ra !== rb) parent.set(ra, rb); };
    const degree = new Map();
    const bump = (k, n) => degree.set(k, (degree.get(k) || 0) + n);

    for (const c of model.components) for (const p of terminals(c)) bump(add(p), 1);
    for (const w of model.wires) {
      const a = add([w.x1, w.y1]), b = add([w.x2, w.y2]);
      union(a, b); bump(a, 1); bump(b, 1);
    }
    // T-junctions: a terminal or wire end lying on another wire joins it.
    // Points are indexed by row and column, so horizontal and vertical wires only test the
    // points on their own line (fast for large drawings); other wires test every point.
    const byRow = new Map(), byCol = new Map();
    for (const [k, p] of coords) {
      if (!byRow.has(p[1])) byRow.set(p[1], []);
      if (!byCol.has(p[0])) byCol.set(p[0], []);
      byRow.get(p[1]).push(k); byCol.get(p[0]).push(k);
    }
    for (const w of model.wires) {
      const a = key([w.x1, w.y1]), b = key([w.x2, w.y2]);
      const candidates = w.y1 === w.y2 ? byRow.get(w.y1) : w.x1 === w.x2 ? byCol.get(w.x1) : [...coords.keys()];
      for (const k of candidates) {
        if (k !== a && k !== b && onSegment(coords.get(k), w)) { union(k, a); bump(k, 2); }
      }
    }
    // All ground symbols are the same node
    let gnd = null;
    for (const c of model.components) {
      if (c.type !== 'GND') continue;
      const k = key([c.x, c.y]);
      if (gnd === null) gnd = k; else union(k, gnd);
    }

    const rootNet = new Map();
    const nets = [];
    const pointNet = new Map();
    for (const k of parent.keys()) {
      const r = find(k);
      if (!rootNet.has(r)) { rootNet.set(r, nets.length); nets.push({ points: [], terminals: 0, ground: false }); }
      const i = rootNet.get(r);
      nets[i].points.push(coords.get(k));
      pointNet.set(k, i);
    }
    if (gnd !== null) nets[pointNet.get(gnd)].ground = true;
    for (const c of model.components) {
      if (c.type === 'GND') continue;
      for (const p of terminals(c)) nets[pointNet.get(key(p))].terminals++;
    }
    return { pointNet, nets, degree };
  }

  // ------------------------------------------------------------ netlist

  // Returns { text, lines, errors: [], warnings: [], phases, nodeCount (drawing nodes), netlistNodes,
  //           labels: [{node, x, y}], componentNodes: Map(id -> [from, to] drawing nodes),
  //           dangling: [[x,y]...], junctions: [[x,y]...] }
  function buildNetlist(model, opts) {
    opts = opts || {};
    const phases = model.phases === 3 ? 3 : 1;
    const errors = [], warnings = [];
    const { pointNet, nets, degree } = connectivity(model);

    // Number the nets that touch a component (ground = 0), left to right, then top to bottom
    const numbered = nets
      .map((n, i) => ({ i, n }))
      .filter(({ n }) => !n.ground && n.terminals > 0)
      .map(({ i, n }) => {
        const lead = n.points.reduce((a, b) => (b[0] < a[0] || (b[0] === a[0] && b[1] < a[1]) ? b : a));
        return { i, lead };
      })
      .sort((a, b) => a.lead[0] - b.lead[0] || a.lead[1] - b.lead[1]);
    const netNode = new Map();
    nets.forEach((n, i) => { if (n.ground) netNode.set(i, 0); });
    const labels = [];
    numbered.forEach(({ i, lead }, j) => { netNode.set(i, j + 1); labels.push({ node: j + 1, x: lead[0], y: lead[1] }); });
    const nodeAt = (p) => netNode.get(pointNet.get(key(p)));
    // Netlist node of drawing node n, phase p (0, 1, 2)
    const pn = (n, p) => (n === 0 ? 0 : phases === 3 ? 3 * (n - 1) + p + 1 : n);

    const parts = model.components.filter((c) => c.type !== 'GND');
    if (parts.length === 0) errors.push('The circuit is empty: add some components.');
    if (!model.components.some((c) => c.type === 'GND')) errors.push('No ground: add a ground symbol (node 0).');
    if (!parts.some((c) => c.type === 'V' || c.type === 'V3')) warnings.push('There is no voltage source.');

    const names = new Map();
    for (const c of parts) names.set(c.name, (names.get(c.name) || 0) + 1);
    for (const [n, k] of names) if (k > 1) warnings.push('The name ' + n + ' is used ' + k + ' times.');

    // Unconnected terminals: alone in their net
    const dangling = [];
    for (const c of parts) {
      for (const p of terminals(c)) {
        const net = nets[pointNet.get(key(p))];
        if (!net.ground && net.terminals < 2) {
          dangling.push(p);
          warnings.push(c.name + ': a terminal is not connected to anything.');
        }
      }
    }

    const lines = [];
    const componentNodes = new Map();
    for (const c of parts) {
      const T = TYPES[c.type];
      if (!T) { errors.push(c.name + ': unknown component type ' + c.type + '.'); continue; }
      if (!!T.three !== (phases === 3)) {
        errors.push(c.name + ' is a ' + (T.three ? 'three' : 'single') + '-phase component in a ' +
                    (phases === 3 ? 'three' : 'single') + '-phase drawing.');
        continue;
      }
      const ts = terminals(c);
      const a = nodeAt(ts[0]), b = ts.length > 1 ? nodeAt(ts[1]) : 0;
      componentNodes.set(c.id, [a, b]);
      if (ts.length > 1 && a === b) { errors.push(c.name + ' is short-circuited: both ends are on node ' + a + '.'); continue; }
      const P = c.params || {};
      const bad = (what) => errors.push(c.name + ': ' + what);
      const num = (k, label, optional) => {
        const v = parseValue(P[k]);
        if (v === null) { if (!optional) bad(label + ' is missing.'); return null; }
        if (Number.isNaN(v)) { bad(label + ' "' + P[k] + '" is not a number.'); return undefined; }
        return v;
      };
      const times = (k, label) => {
        const ts2 = parseList(P[k]);
        if (ts2.some((t) => Number.isNaN(t))) { bad(label + ' "' + P[k] + '" are not all numbers.'); return null; }
        return ts2;
      };
      const tag = (p) => '   # ' + c.name + (p == null ? '' : '.' + PH[p]);
      const phaseList = phases === 3 ? [0, 1, 2] : [null];

      if (['R', 'L', 'C', 'R3', 'L3', 'C3'].includes(c.type)) {
        const base = T.base || c.type;
        const v = num('value', T.params[0].label);
        if (v == null) continue;
        if (v <= 0) { bad(T.params[0].label + ' must be positive.'); continue; }
        for (const p of phaseList) lines.push(base + ' ' + pn(a, p || 0) + ' ' + pn(b, p || 0) + '  ' + fmt(v) + tag(p));
      } else if (c.type === 'V' || c.type === 'V3') {
        const mag = num('mag', 'Peak voltage'), ang = num('angle', 'Angle'), f = num('freq', 'Frequency');
        const r = num('r', 'Internal resistance', true);
        if (mag == null || ang == null || f == null || r === undefined) continue;
        const ideal = !(r > 0);
        if (ideal && a !== 0 && b !== 0) { bad('an ideal source (no internal resistance) needs one terminal on ground.'); continue; }
        const shift = [0, -120, 120];
        for (const p of phaseList) {
          lines.push('V ' + pn(a, p || 0) + ' ' + pn(b, p || 0) + '  ' + fmt(mag) + ' ' + fmt(ang + shift[p || 0]) + ' ' + fmt(f) +
                     (ideal ? '' : ' ' + fmt(r)) + tag(p));
        }
      } else if (c.type === 'SW' || c.type === 'SW3') {
        const indep = c.type === 'SW3' && P.mode === 'independent';
        let ok = true;
        const out = [];
        for (const p of phaseList) {
          const sk = indep ? 'state_' + PH[p] : 'state', tk = indep ? 'times_' + PH[p] : 'times';
          const tl = times(tk, (indep ? 'pole ' + PH[p] + ' ' : '') + 'switching times');
          if (!tl) { ok = false; break; }
          const state = P[sk] === 'open' ? 'open' : 'closed';
          out.push('SW ' + pn(a, p || 0) + ' ' + pn(b, p || 0) + '  ' + state + (tl.length ? ' ' + tl.map(fmt).join(' ') : '') + tag(p));
        }
        if (ok) lines.push(...out);
      } else if (c.type === 'F') {
        const t0 = num('t_apply', 'Applied at'), t1 = num('t_clear', 'Cleared at', true);
        if (t0 == null || t1 === undefined) continue;
        if (t1 != null && t1 <= t0) { bad('must be cleared after it is applied.'); continue; }
        lines.push('F ' + a + ' ' + b + '  ' + fmt(t0) + (t1 != null ? ' ' + fmt(t1) : '') + tag(null));
      } else if (c.type === 'F3') {
        if (a === 0) { bad('a fault must be on a bus, not on ground.'); continue; }
        const kind = String(P.kind || 'a-g');
        const ground = kind.endsWith('-g');
        const which = kind.replace('-g', '').split('').map((ch) => PH.indexOf(ch));
        if (!which.length || which.some((p) => p < 0)) { bad('unknown fault type "' + kind + '".'); continue; }
        if (!ground && which.length < 2) { bad('a fault without ground needs at least two phases.'); continue; }
        const indep = P.mode === 'independent';
        const timing = {};
        let ok = true;
        for (const p of which) {
          const sfx = indep ? '_' + PH[p] : '';
          const lab = indep ? 'Phase ' + PH[p] + ': ' : '';
          const t0 = num('t_apply' + sfx, lab + 'applied at'), t1 = num('t_clear' + sfx, lab + 'cleared at', true);
          if (t0 == null || t1 === undefined) { ok = false; break; }
          if (t1 != null && t1 <= t0) { bad(lab + 'must be cleared after it is applied.'); ok = false; break; }
          timing[p] = fmt(t0) + (t1 != null ? ' ' + fmt(t1) : '');
        }
        if (!ok) continue;
        if (ground) {
          for (const p of which) lines.push('F ' + pn(a, p) + ' 0  ' + timing[p] + tag(p));
        } else {
          // phase-to-phase: a switch between consecutive faulted phases (ab, bc; abc -> ab + bc),
          // timed by the first phase of each pair
          for (let k = 0; k + 1 < which.length; k++) {
            const p = which[k], q = which[k + 1];
            lines.push('F ' + pn(a, p) + ' ' + pn(a, q) + '  ' + timing[p] + '   # ' + c.name + '.' + PH[p] + PH[q]);
          }
        }
      }
    }

    const junctions = [];
    for (const [k, d] of degree) if (d >= 3) junctions.push(k.split(',').map(Number));

    const N = numbered.length;
    const header = [
      '# EMTSim netlist' + (opts.title ? ': ' + opts.title : '') + ' (drawn with the EMTSim circuit editor)',
      '#',
      '# Format: R/L/C from to value | V from to peak angle freq [R] | SW from to closed|open times... | F from to t_apply [t_clear]',
      '# ' + phases * N + ' nodes' + (phases === 3 ? ' (' + N + ' three-phase nodes)' : '') + ', ' + lines.length + ' elements',
    ];
    if (phases === 3) {
      header.push('# Three-phase drawing: node k of the drawing is netlist node 3k-2 (phase a), 3k-1 (b) and 3k (c)');
    }
    header.push('');
    const layout = '#@layout ' + JSON.stringify(serialize(model));
    const text = header.concat(lines, ['', layout]).join('\n') + '\n';
    return { text, lines, errors, warnings, phases, nodeCount: N, netlistNodes: phases * N, labels,
             componentNodes, dangling, junctions };
  }

  // ------------------------------------------------------------ files

  function serialize(model) {
    const out = {
      v: 1,
      components: model.components.map((c) => ({ type: c.type, name: c.name, x: c.x, y: c.y, rot: c.rot || 0, params: c.params || {} })),
      wires: model.wires.map((w) => ({ x1: w.x1, y1: w.y1, x2: w.x2, y2: w.y2 })),
    };
    if (model.phases === 3) out.phases = 3;
    return out;
  }

  let nextId = 1;
  function newId() { return 'c' + (nextId++); }

  // Netlist text -> { model } if it has a drawing (#@layout line), else { raw: text }
  function parseFile(text) {
    const line = text.split(/\r?\n/).find((l) => l.startsWith('#@layout '));
    if (!line) return { raw: text };
    const data = JSON.parse(line.slice('#@layout '.length));
    return {
      model: {
        v: 1,
        phases: data.phases === 3 ? 3 : 1,
        components: (data.components || []).map((c) => ({ id: newId(), type: c.type, name: c.name, x: c.x, y: c.y, rot: c.rot || 0, params: Object.assign({}, c.params) })),
        wires: (data.wires || []).map((w) => ({ x1: w.x1, y1: w.y1, x2: w.x2, y2: w.y2 })),
      },
    };
  }

  function defaults(type) {
    const params = {};
    for (const p of TYPES[type].params) params[p.key] = p.def;
    return params;
  }

  function makeComponent(type, x, y, rot, existing) {
    const t = TYPES[type];
    const used = new Set((existing || []).map((c) => c.name));
    let n = 1;
    while (used.has(t.prefix + n)) n++;
    return { id: newId(), type, name: type === 'GND' ? 'GND' + n : t.prefix + n, x, y, rot: rot || 0, params: defaults(type) };
  }

  // Convert a drawing between single- and three-phase (returns a new model).
  //   1 -> 3: R, L, C, V, SW become their 3-phase versions with the same values (switch poles together).
  //           A fault F becomes a 3-phase fault F3 (type abc-g) at its first terminal.
  //   3 -> 1: the reverse; independent switch poles and fault phases use phase a's settings,
  //           and a fault F3 becomes F to a new ground symbol below it.
  function convertPhases(model, phases) {
    const from = model.phases === 3 ? 3 : 1;
    const out = { v: 1, phases, components: [], wires: model.wires.map((w) => Object.assign({}, w)) };
    if (from === phases) { out.components = model.components.map((c) => Object.assign({}, c, { params: Object.assign({}, c.params) })); return out; }
    for (const c of model.components) {
      const P = c.params || {};
      if (c.type === 'GND') { out.components.push(Object.assign({}, c)); continue; }
      const type = phases === 3 ? TO3[c.type] : TO1[c.type];
      if (!type) { out.components.push(Object.assign({}, c, { params: Object.assign({}, P) })); continue; }
      const params = defaults(type);
      for (const k of Object.keys(params)) if (P[k] !== undefined) params[k] = P[k];
      const n = { id: c.id, type, name: c.name, x: c.x, y: c.y, rot: c.rot || 0, params };
      if (phases === 1 && P.mode === 'independent') {            // take phase a's settings
        if (c.type === 'SW3') { params.state = P.state_a; params.times = P.times_a; }
        if (c.type === 'F3') { params.t_apply = P.t_apply_a; params.t_clear = P.t_clear_a; }
      }
      if (type === 'F3') { params.kind = 'abc-g'; n.rot = 0; }
      if (type === 'F') {                                          // F3 (single terminal) -> F down to ground
        n.rot = 90;
        out.components.push(n);
        out.components.push({ id: newId(), type: 'GND', name: 'GND', x: c.x, y: c.y + LEN, rot: 0, params: {} });
        continue;
      }
      out.components.push(n);
    }
    return out;
  }

  return { LEN, DIRS, PH, TYPES, TO3, TO1, isThree, activeParams, singleTerminal, parseValue, parseList, fmt, formatSI,
           terminals, onSegment, connectivity, buildNetlist, serialize, parseFile, makeComponent, newId, convertPhases };
});
