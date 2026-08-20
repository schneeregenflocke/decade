# decade

Desktop application for calendars and timelines. The page exports as a PNG; data comes from CSV, whole projects from XML.

Alpha state, grown out of a practice project. C++26, Qt 6, OpenGL, ICU.

Building, starting, tests and the headless runs stand in [operations.md](operations.md); architecture and conventions in [AGENTS.md](AGENTS.md).

## Purpose

### The interface

The input tables and settings on the left, the rendered page on the right:

![The main window with the data table and the calendar preview](assets/screenshot.png)

### Export

The same page as a PNG export at print resolution:

![The calendar page exported as a PNG](assets/render.png)
