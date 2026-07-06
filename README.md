# LLC-GaN-Protoboard

This board will be a prototype GaN based LLC converter, with a target rating of 80W with 24V input to 400V output, ideally for renewable power conversion and high voltage applications in plasma generation. 

This board will be controlled by and mounted on top of the [LAUNCHXL-F28P55X](https://www.digikey.com/en/products/detail/texas-instruments/LAUNCHXL-F28P55X/22674628). It will be based on the reference design for TIDA-010062, minus the PFC element.

The board is made in KiCad while the code is done in Code Composer Studio by TI. 

Some resources that were used for reference during board design:
- [Pinout Map](https://www.ti.com/lit/ml/spaz056/spaz056.pdf?ts=1783172662507)
- [LaunchPad Hardware Design Guidelines](https://www.ti.com/lit/an/slaa542/slaa542.pdf)
- [F28P55X Datasheet](https://www.ti.com/lit/ds/sprsp85d/sprsp85d.pdf?ts=1783173915619)
- [Open Magnetics](https://openmagnetics.com)

- [LLC Topology](https://e2e.ti.com/cfs-file/__key/communityserver-discussions-components-files/196/Ch4.pdf)
- [LLC Switches and Tank](https://www.monolithicpower.com/learning/resources/understanding-llc-operation-part-i-power-switches-and-resonant-tank)
- [Example LLC Design](https://www.infineon.com/assets/row/public/documents/24/42/infineon-design-example-resonant-llc-converter-operation-and-design-applicationnotes-en.pdf)