# Network stack

**Requirements**:
- Hardware based MAC address filtering (otherwise it requires source code modification).
- One 32-bit 1 ms timer.
- Send raw network frame.
- Receive raw network frame.

**Reccomendations**:
- More than 32k RAM (depending on network load).

**Implemented**:
- Completely interrupt free operations.
- Zero copy support on all protocols.
- Template driver for SAM4E.
