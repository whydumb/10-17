# Debugging Web Platform

Debugging a web version in the browser is not as seamless as Visual Studio, but we can configure **VSCode** to provide a similar experience.

This guide is based on the following **reference**: [WASM Debugging with Emscripten and VSCode](https://floooh.github.io/2023/11/11/emscripten-ide.html).

---

## Setup

We will be using **VSCode + CMake + Emscripten**.

### Prerequisites

1. **Install [VSCode](https://code.visualstudio.com/Download).**
2. **Set up CMake and Emscripten** by following the [Development Environment](DevEnvironment.md) guide.

### Required VSCode Extensions

Ensure you have the following extensions installed:

- `ms-vscode.cpptools` (C++ Tools)
- `ms-vscode.cmake-tools` (CMake Integration)
- `ms-vscode.wasm-dwarf-debugging` (WebAssembly Debugging)
- `ms-vscode.live-server` (Local Server for Debugging)

---

## Debugging Steps

1. **Open your project folder in VSCode.**
2. Open the command palette:
   - **Windows/Linux**: `Ctrl + Shift + P`
   - **macOS**: `Cmd + Shift + P`
3. Search for **CMake: Select Configure Preset** and choose `Web Debug` or `Web Debug on Windows`.
4. Open the `Run & Debug` tab (`Ctrl + Shift + D`).
5. In the dropdown, select the browser for debugging (`Chrome` or `Edge`).
6. Start the debugging session by:
   - Pressing `F5`, or
   - Clicking the green play button.
7. This launches a local web server on port `3000` and opens the browser to load your HTML page for debugging.
8. **Set a breakpoint** in your code while it's running to step through execution.

Now you can debug your web project just like in a native environment!

---

## Using the Browser Console for Debugging Output

In addition to setting breakpoints, you can use the browser’s developer console to view debug output and log messages.

### Opening the Developer Console:
- **Google Chrome** or **Microsoft Edge**:
  1. Open the browser.
  2. Press `F12` or `Ctrl + Shift + I` (`Cmd + Option + I` on macOS).
  3. Click on the **Console** tab.

### Viewing Debug Output:

- Using functions like `printf`, `std::cout` and our own logging system messages will be shown here.
- You can also check for warnings, errors, and network requests in the console to help diagnose issues.
- **If the webpage isn't behaving as you expect then this should be the first place to look for hints**

---

By using the browser console alongside VSCode debugging, you can gain more insights into your program’s execution and troubleshoot effectively.
