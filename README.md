# Blender Editor Controls

![Workflow Demonstration](placeholder.gif)

**Blender Editor Controls** is a free, source-available plugin for Unreal Engine that brings Blender's transformation workflow (G, R, S) directly into the Unreal Editor viewport. 

This plugin bypasses the standard Unreal Engine gizmo widgets, allowing you to quickly move, rotate, and scale objects using keyboard-driven modal operations, axis constraints, and numeric inputs.

[**Get it for free on Fab**](#) (WAITING FOR SUBMISSION APPROVAL)

## Table of Contents
- [Blender Editor Controls](#blender-editor-controls)
  - [Table of Contents](#table-of-contents)
  - [Installation](#installation)
  - [Features](#features)
  - [Architecture Overview (For Contributors)](#architecture-overview-for-contributors)
  - [Usage Guide \& Hotkeys](#usage-guide--hotkeys)
  - [Settings \& Customization](#settings--customization)
  - [Compatibility](#compatibility)
  - [License](#license)

## Installation

**Method 1: Git Clone (Recommended)**

1. Navigate to the root directory of your Unreal Engine project (where your `.uproject` file is located).
2. If it does not exist, create a new folder named `Plugins`.
3. Open a terminal inside the `Plugins` folder.
4. Run the command: `git clone [https://github.com/jefimh/BlenderEditorControlsUE5.git](https://github.com/jefimh/BlenderEditorControlsUE5.git)`
5. Open your Unreal Engine project. It will ask to rebuild the plugin modules; click "Yes".
6. Note: This plugin should be enabled by default. If for any reason it is not active, navigate to **Edit > Plugins**, search for "Blender Editor Controls", and ensure the box is checked.

**Method 2: ZIP Download**

1. Download the source code as a ZIP file.
2. Extract the archive.
3. Navigate to the root directory of your Unreal Engine project (where your `.uproject` file is located) and create a `Plugins` folder if it does not exist.
4. Move the extracted folder into your `Plugins/` directory.
5. Open your Unreal Engine project. It will ask to rebuild the plugin modules; click "Yes".
6. Note: This plugin should be enabled by default. If for any reason it is not active, navigate to **Edit > Plugins**, search for "Blender Editor Controls", and ensure the box is checked.

---

*(Note for C++ developers: You can also right-click your `.uproject` file, select "Generate Visual Studio project files", and compile manually via your IDE).*

---

## Features

- **Blender-Style Hotkeys**: Use **G** (Grab/Translate), **R** (Rotate), and **T** (Scale) to immediately start transforming your selection without needing to click or drag gizmo arrows.
- **Mid-Session Tool Switching**: Seamlessly switch between Move, Rotate, and Scale during an active transformation without needing to cancel or click out.
- **Infinite Cursor Wrapping**: Drag endlessly! When your cursor hits the edge of the viewport during an operation, it seamlessly wraps around to the other side.
- **Axis Locking**: Lock transformations to specific axes or planes by pressing **X**, **Y**, or **Z** during an operation. Pressing the axis key twice toggles between Global and Local space.
- **Numeric Input**: Type values directly during an operation for precise adjustments (e.g., press `G`, `X`, type `15.5`, and press `Enter`). Supports unit-aware math parsing.
- **Trackball Rotation**: Press **R** twice to enter freeform Trackball rotation mode.
- **Duplicate & Move**: Press **Shift+D** to duplicate the current actor selection and immediately begin moving it.
- **Advanced Editor Snapping**: Natively supports standard Unreal Engine vertex snapping (hold `V`) and snapping to other actors.
- **Multi-Context Support**: Works seamlessly across:
  - Standard Level Editor Actors (fully supports Orthographic Viewports)
  - Blueprint Components (SCS Tree Nodes)
  - Control Rig Elements (Bones, Controls)
- **Undo & Redo**: Native editor `Ctrl+Z` and `Ctrl+Y` are completely supported for all operations.

## Architecture Overview (For Contributors)

Contributions are highly welcomed! To help you get up to speed, here is a high-level overview of the codebase structure found in the `Source/` directory. Detailed API documentation is also available via Doxygen comments directly in the source headers.

- [**`InputProcessor`**](Source/BlenderEditorControlsPlugin/Public/Input/InputProcessor.h): The gatekeeper. It hooks into the Slate application to intercept hotkeys (G, R, S, Shift+D) before they reach the viewport and triggers a new transform session.
- [**`TransformSession`**](Source/BlenderEditorControlsPlugin/Public/TransformSession.h): The core state machine. It manages the active tool's lifecycle, caches initial mouse/camera states, and handles Unreal Engine's `FScopedTransaction` to ensure Undo/Redo works flawlessly.
- [**`Tools/`**](Source/BlenderEditorControlsPlugin/Public/Tools/): Contains the logic for specific operations (`MoveTool`, `RotateTool`, `ScaleTool`). They calculate math based on mouse deltas and send the values to the pivots.
- [**`Pivots/`**](Source/BlenderEditorControlsPlugin/Public/Pivots/): Abstractions that handle the actual application of transforms to different object types. For example, `ActorPivot` handles standard actors, while `ControlRigPivot` safely interfaces with RigVM.
- [**`Input/Numeric/`**](Source/BlenderEditorControlsPlugin/Public/Input/Numeric/): Manages keyboard-driven value inputs during an active session, interpreting units and math formulas typed by the user.
- [**`UI/`**](Source/BlenderEditorControlsPlugin/Public/UI/): The screen-space widget (`TransformHUD`) that displays active values, and the `AxisLockGizmoComponent` responsible for drawing the infinite colored lines.

## Usage Guide & Hotkeys

| Action | Shortcut | Description |
| :--- | :--- | :--- |
| **Translate (Grab)** | `G` | Moves the selection relative to the screen plane. |
| **Rotate** | `R` | Rotates the selection relative to the view angle. |
| **Trackball Rotate** | `R` then `R` | Rotates freely in all directions. |
| **Scale** | `T` | Scales the selection uniformly. Set to T instead of S to not clash with native editor bindings |
| **Duplicate** | `Shift + D` | Duplicates actors and begins moving them. |
| **Lock Axis (Global)** | `X`, `Y`, or `Z` | Locks the transform to the X, Y, or Z world axis. |
| **Lock Axis (Local)**| Double tap `X`, `Y`, `Z` | Locks the transform to the local coordinate axis. |
| **Lock Plane** | `Shift + X, Y, Z` | Locks the transform to a 2D plane (e.g., Shift+Z locks to XY floor). |
| **Confirm** | `Left Click`, `Enter`, or `Space` | Applies the transformation. |
| **Cancel** | `Right Click` or `Esc` | Reverts the selection to its original state. |
| **Precision Mode** | Hold `Shift` | Slows down mouse influence for fine adjustments. |
| **Toggle Snapping** | Hold `Ctrl` | Inverts the current viewport grid-snapping state. |

## Settings & Customization

You can customize the plugin's behavior, including axis colors, precision scalars, and 3D line thickness. 
To access the settings:
1. Go to **Edit > Editor Preferences**.
2. Scroll down to the **Plugins** section.
3. Select **Blender Editor Controls**.

You can also customize the exact keybindings (such as changing Scale back to `S`) by navigating to **Edit > Editor Preferences > General > Keyboard Shortcuts** and searching for **Blender Editor Controls**.

## Compatibility

- Supported Unreal Engine versions: **5.6 - 5.8**

## License

This project is licensed under the **Creative Commons Attribution-NonCommercial 4.0 International (CC BY-NC 4.0)**. 

You are free to share and adapt this plugin for non-commercial purposes, provided that you give appropriate credit to **jefimh** (including a link to this repository: [https://github.com/jefimh/BlenderEditorControlsUE5](https://github.com/jefimh/BlenderEditorControlsUE5)), provide a link to the license, and indicate if changes were made. Commercial redistribution is strictly prohibited.
