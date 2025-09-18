# Hardware (KiCad)

## Overview
- Project directory: `hardware_kicad/`
- Core sources: schematics (`*.kicad_sch`), board (`*.kicad_pcb`), project file (`*.kicad_pro`).
- Auxiliary: iBOM config (`ibom.config.ini`), optional STL and notes.

Prototype hardware for the trigger has been validated against the v1.21a firmware.

## KiCad Version
- Designed/tested with KiCad 7/8. Re‑generate outputs with your installed version.

## Fabrication Outputs
To produce fabrication outputs (Gerbers/Drill/PDF):
- Open the project in KiCad.
- Plot layers as required by your fabricator (F.Cu/B.Cu, F.SilkS, F.Mask, Edge.Cuts, etc.).
- Generate drill files (PTH/NPTH) and drill maps as needed.
- Optionally export PDFs for documentation and assembly guides.

Note: historical “plotter outputs” and iBOM HTMLs may exist from prototyping. Future releases will attach curated artifacts to GitHub releases rather than committing bulk outputs.

## Safety Notes
- Treat the HV trigger output as live hardware — validate with dummy loads first.
- Verify pin mappings in `config.h` for your ESP32 variant before powering hardware.

