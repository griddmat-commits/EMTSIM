// Turn a schematic drawing (JSON) into an EMTSim netlist, exactly as the circuit editor does.
//   node gui/layout_to_net.js drawing.json "title" > circuit.net
// Exits with status 1 and lists the problems if the drawing has errors.
'use strict';
const fs = require('fs');
const S = require('./schematic.js');

const data = JSON.parse(fs.readFileSync(process.argv[2], 'utf8'));
const model = S.parseFile('#@layout ' + JSON.stringify(data)).model;
const nl = S.buildNetlist(model, { title: process.argv[3] || '' });
for (const w of nl.warnings) console.error('warning: ' + w);
if (nl.errors.length) {
  for (const e of nl.errors) console.error('error: ' + e);
  process.exit(1);
}
process.stdout.write(nl.text);
